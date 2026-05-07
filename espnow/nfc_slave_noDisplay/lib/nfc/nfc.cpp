#include "nfc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const uint32_t LOG_INTERVAL_MS = 1000;

static SemaphoreHandle_t s_spiMutex = nullptr;
static bool s_spiInited = false;
static uint8_t s_spiSck = 0;
static uint8_t s_spiMiso = 0;
static uint8_t s_spiMosi = 0;

static void ensureSpiMutex()
{
  if (!s_spiMutex) {
    s_spiMutex = xSemaphoreCreateRecursiveMutex();
  }
}

class SpiLock
{
public:
  SpiLock()
  {
    ensureSpiMutex();
    _locked = (s_spiMutex && xSemaphoreTakeRecursive(s_spiMutex, pdMS_TO_TICKS(50)) == pdTRUE);
  }
  ~SpiLock()
  {
    if (_locked && s_spiMutex) {
      xSemaphoreGiveRecursive(s_spiMutex);
    }
  }
  bool ok() const { return _locked; }

private:
  bool _locked = false;
};

static const char *statusName(MFRC522::StatusCode code)
{
  switch (code)
  {
  case MFRC522::STATUS_OK: return "OK";
  case MFRC522::STATUS_ERROR: return "ERROR";
  case MFRC522::STATUS_COLLISION: return "COLLISION";
  case MFRC522::STATUS_TIMEOUT: return "TIMEOUT";
  case MFRC522::STATUS_NO_ROOM: return "NO_ROOM";
  case MFRC522::STATUS_INTERNAL_ERROR: return "INTERNAL_ERROR";
  case MFRC522::STATUS_INVALID: return "INVALID";
  case MFRC522::STATUS_CRC_WRONG: return "CRC_WRONG";
  case MFRC522::STATUS_MIFARE_NACK: return "MIFARE_NACK";
  default: return "UNKNOWN";
  }
}

NfcReader::NfcReader(const NfcConfig &config)
    : _settings(config.settings),
      _sck(config.pin.SCK),
      _miso(config.pin.MISO),
      _mosi(config.pin.MOSI),
      _ss(config.pin.SS),
      _rst(config.pin.RST),
      _irq(config.pin.IRQ),
      _rfid(config.pin.SS, config.pin.RST),
      _task(config.task)
{
  if (_settings.pollIntervalMs == 0)
    _settings.pollIntervalMs = 100;
  if (_settings.reinitMisses == 0)
    _settings.reinitMisses = 8;

  _irqMode = _settings.irqMode;
  if (_irqMode && _irq < 0)
  {
    _irqMode = false;
  }

  if (_settings.miss >= 0)
    _removeTimeMs = static_cast<uint16_t>(_settings.miss > 65535 ? 65535 : _settings.miss);
}

void NfcReader::begin()
{
  pinMode(_ss, OUTPUT);
  digitalWrite(_ss, HIGH);
  pinMode(_rst, OUTPUT);
  digitalWrite(_rst, HIGH);
  if (_irq >= 0) {
    pinMode(_irq, INPUT_PULLUP);
  }
  delay(50);

  {
    SpiLock lock;
    if (!lock.ok())
    {
      _lastError = "SPI Mutex";
      if (_onError) _onError(_lastError);
      return;
    }
    if (!s_spiInited)
    {
      SPI.begin(_sck, _miso, _mosi, _ss);
      s_spiInited = true;
      s_spiSck = _sck;
      s_spiMiso = _miso;
      s_spiMosi = _mosi;
    }
    else
    {
      (void)s_spiSck;
      (void)s_spiMiso;
      (void)s_spiMosi;
    }

    _rfid.PCD_Init();
    delay(10);
    _rfid.PCD_AntennaOn();
    _rfid.PCD_SetAntennaGain(MFRC522::RxGain_23dB_2);
  }

  _ok = checkHardware();
  _lastCheckMs = millis();
  if (!_ok) {
    _lastError = "NFC Error";
    if (_onError) _onError(_lastError);
  }

  _lastUid = "";
  _cardPresent = false;
  _failCount = 0;
  _missCount = 0;
  _lastPollMs = 0;
  _lastSeenMs = 0;

  if (_task) {
    _task->begin({
      .loop = nullptr, // 컨텍스트 루프를 사용
      .loopWithContext = NfcReader::taskThunk, // TaskRunner에서 호출할 루프 함수
      .context = this, // 루프 함수로 전달할 컨텍스트
    });
  }
}

void NfcReader::begin(const Callbacks &cbs)
{
  setCallbacks(cbs);
  begin();
}

void NfcReader::reset()
{
  SpiLock lock;
  if (!lock.ok())
    return;

  _rfid.PCD_StopCrypto1();
  _rfid.PCD_Reset();
  delay(10);
  _rfid.PCD_Init();
  _rfid.PCD_AntennaOn();
  _rfid.PCD_SetAntennaGain(MFRC522::RxGain_23dB_2);
  _failCount = 0;
}

void NfcReader::onRead(ReadCallback cb) { _onRead = cb; }
void NfcReader::onRemove(RemoveCallback cb) { _onRemove = cb; }
void NfcReader::onError(ErrorCallback cb) { _onError = cb; }
void NfcReader::setCallbacks(const Callbacks &cbs)
{
  _onRead = cbs.onRead;
  _onRemove = cbs.onRemove;
  _onError = cbs.onError;
}
uint8_t NfcReader::version() const { return _version; }
bool NfcReader::hasError() const { return !_lastError.isEmpty(); }
const String &NfcReader::lastError() const { return _lastError; }
const String &NfcReader::lastUid() const { return _lastUid; }

void NfcReader::setEnabled(bool enabled)
{
  if (_enabled == enabled)
    return;
  _enabled = enabled;
  _lastEnableMs = millis();

  SpiLock lock;
  if (!lock.ok())
    return;

  if (_enabled) {
    _rfid.PCD_AntennaOn();
  } else {
    _rfid.PCD_AntennaOff();
  }
}

void NfcReader::taskThunk(void *context)
{
  static_cast<NfcReader *>(context)->taskTick();
}

void NfcReader::taskTick()
{
  if (!_enabled)
  {
    delay(5);
    return;
  }
  // 안테나 전환 직후 짧은 안정화 시간
  if (millis() - _lastEnableMs < 40)
  {
    delay(5);
    return;
  }
  if (millis() - _lastLogMs > LOG_INTERVAL_MS)
  {
    _lastLogMs = millis();
  }

  if (!_ok)
  {
    if (millis() - _lastCheckMs > 1000)
    {
      _lastCheckMs = millis();
      reset();
      _ok = checkHardware();
      if (!_ok)
      {
        _lastError = "NFC Error";
        if (_onError) _onError(_lastError);
      }
      if (_ok)
      {
        _lastError = "";
        // 하드웨어 복구 시점에는 실제 카드 제거를 보장할 수 없으므로 onRemove 호출 안 함
      }
    }
    delay(20);
    return;
  }

  bool irqAsserted = (_irq >= 0 && digitalRead(_irq) == LOW);
  bool readAllowed = (!_irqMode) || (_irqMode && irqAsserted);
  if (!readAllowed)
  {
    delay(5);
    return;
  }
  if (!irqAsserted && (millis() - _lastPollMs < _settings.pollIntervalMs))
  {
    delay(5);
    return;
  }
  _lastPollMs = millis();

  SpiLock lock;
  if (!lock.ok())
  {
    delay(5);
    return;
  }

  byte atqa[2];
  byte atqaLen = sizeof(atqa);
  MFRC522::StatusCode req = _rfid.PICC_WakeupA(atqa, &atqaLen);
  if (req != MFRC522::STATUS_OK)
  {
    if (_lastReqStatus != static_cast<uint8_t>(req))
    {
      _lastReqStatus = static_cast<uint8_t>(req);
    }
    if (_cardPresent)
    {
      if (_settings.miss >= 0)
      {
        if (_removeTimeMs == 0 || (_lastSeenMs > 0 && (millis() - _lastSeenMs) >= _removeTimeMs))
        {
          _cardPresent = false;
          _lastUid = "";
          if (_onRemove)
            _onRemove();
        }
      }
    }
    reinitIfNeeded();
    delay(5);
    return;
  }

  _cardPresent = true;
  _missCount = 0;
  _failCount = 0;
  // 카드가 감지되면 존재 시간 갱신 (UID 읽기 실패 시에도 유지)
  _lastSeenMs = millis();

  if (!_rfid.PICC_ReadCardSerial())
  {
    reinitIfNeeded();
    delay(5);
    return;
  }

  String uid = uidToHex(_rfid.uid.uidByte, _rfid.uid.size);
  if (uid != _lastUid)
  {
    _lastUid = uid;
    if (_onRead)
      _onRead(uid);
  }

  delay(5);
}

String NfcReader::uidToHex(const uint8_t *uid, uint8_t uidLength)
{
  String s;
  for (uint8_t i = 0; i < uidLength; i++)
  {
    if (uid[i] < 0x10)
      s += "0";
    s += String(uid[i], HEX);
    if (i + 1 < uidLength)
      s += ":";
  }
  s.toUpperCase();
  return s;
}

bool NfcReader::checkHardware()
{
  SpiLock lock;
  if (!lock.ok())
    return false;

  _version = _rfid.PCD_ReadRegister(MFRC522::VersionReg);
  return (_version != 0x00 && _version != 0xFF);
}

void NfcReader::reinitIfNeeded()
{
  if (++_failCount >= _settings.reinitMisses)
  {
    reset();
  }
}
