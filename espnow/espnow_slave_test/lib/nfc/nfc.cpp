#include "nfc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const uint32_t LOG_INTERVAL_MS = 1000;
static const uint32_t BOOT_WARMUP_MS = 2500;
static const uint16_t READ_CONFIRM_MS = 90;
static const uint8_t READ_CONFIRM_COUNT = 2;
static const uint16_t REMOVE_PENDING_GRACE_MS = 180;
static const uint16_t EVENT_REFRACTORY_MS = 250;
static const uint16_t ENABLE_REMOVE_SUPPRESS_MS = 180;
static const uint16_t ENABLE_READ_CONFIRM_RESET_MS = 60;
static const uint8_t EVENT_NONE = 0;
static const uint8_t EVENT_READ = 1;
static const uint8_t EVENT_REMOVE = 2;

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
      _rfid(config.pin.SS, config.pin.RST >= 0 ? config.pin.RST : 255), // MFRC522 라이브러리에서 255는 '미사용'을 의미함
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

  if (_settings.miss > 0) {
    _removeTimeMs = static_cast<uint16_t>(_settings.miss > 65535 ? 65535 : _settings.miss);
  } else if (_settings.miss == 0) {
    // REMOVE 디바운싱: 순간적인 미인식으로 인한 거짓 REMOVE 패킷 전송을 막기 위해
    // 설정이 0(즉시)이더라도 기본 500ms 대기 시간을 갖도록 강제 적용합니다.
    _removeTimeMs = 500;
  }
}

void NfcReader::begin()
{
  pinMode(_ss, OUTPUT);
  digitalWrite(_ss, HIGH);
  if (_rst >= 0) { // RST 핀이 -1이 아닐 때만 핀 모드 설정
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
  }
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
  _pendingReadUid = "";
  _cardPresent = false;
  _publishedPresent = false;
  _removePending = false;
  _failCount = 0;
  _missCount = 0;
  _pendingReadCount = 0;
  _lastPollMs = 0;
  _lastSeenMs = 0;
  _pendingReadStartMs = 0;
  _removePendingStartMs = 0;
  _lastEventMs = 0;
  _lastEventType = EVENT_NONE;
  _bootWarmupUntilMs = millis() + BOOT_WARMUP_MS;
  _suppressRemoveUntil = _bootWarmupUntilMs;
  _disabledStartMs = 0;

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
  _missCount = 0;
  _pendingReadUid = "";
  _pendingReadCount = 0;
  _pendingReadStartMs = 0;
  _removePending = false;
  _removePendingStartMs = 0;
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
uint16_t NfcReader::pollIntervalMs() const { return _settings.pollIntervalMs; }
uint16_t NfcReader::removeTimeMs() const { return _removeTimeMs; }

void NfcReader::setEnabled(bool enabled)
{
  if (_enabled == enabled)
    return;
  const uint32_t now = millis();
  _enabled = enabled;
  _lastEnableMs = now;
  if (_enabled) {
    if (_disabledStartMs != 0) {
      shiftTimingWindows(now - _disabledStartMs);
      _disabledStartMs = 0;
    }
    _pendingReadUid = "";
    _pendingReadCount = 0;
    _pendingReadStartMs = 0;
    _suppressRemoveUntil = _lastEnableMs + ENABLE_REMOVE_SUPPRESS_MS;
  } else {
    _disabledStartMs = now;
  }

  SpiLock lock;
  if (!lock.ok())
    return;

  if (_enabled) {
    _rfid.PCD_AntennaOn();
  } else {
    _rfid.PCD_AntennaOff();
  }
}

void NfcReader::shiftTimingWindows(uint32_t deltaMs)
{
  if (deltaMs == 0) {
    return;
  }

  if (_lastSeenMs != 0) {
    _lastSeenMs += deltaMs;
  }
  if (_pendingReadStartMs != 0) {
    _pendingReadStartMs += deltaMs;
  }
  if (_removePendingStartMs != 0) {
    _removePendingStartMs += deltaMs;
  }
  if (_lastEventMs != 0) {
    _lastEventMs += deltaMs;
  }
}

void NfcReader::taskThunk(void *context)
{
  static_cast<NfcReader *>(context)->taskTick();
}

void NfcReader::taskTick()
{
  const uint32_t now = millis();

  if (!_enabled)
  {
    return;
  }
  // 안테나 전환 직후 짧은 안정화 시간
  if (now - _lastEnableMs < 40)
  {
    return;
  }
  if (now - _lastLogMs > LOG_INTERVAL_MS)
  {
    _lastLogMs = now;
  }

  if (!_ok)
  {
    if (now - _lastCheckMs > 1000)
    {
      _lastCheckMs = now;
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
    return;
  }

  bool irqAsserted = (_irq >= 0 && digitalRead(_irq) == LOW);
  if (!irqAsserted && (now - _lastPollMs < _settings.pollIntervalMs))
  {
    return;
  }
  _lastPollMs = now;

  bool cardSeenThisTick = false;
  String seenUid;
  SpiLock lock;
  if (!lock.ok())
  {
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
    reinitIfNeeded();
  }
  else
  {
    cardSeenThisTick = true;
    _cardPresent = true;
    _missCount = 0;
    _failCount = 0;
    _lastReqStatus = static_cast<uint8_t>(req);
    _lastSeenMs = now;

    if (_rfid.PICC_ReadCardSerial())
    {
      seenUid = uidToHex(_rfid.uid.uidByte, _rfid.uid.size);
    }
    else
    {
      reinitIfNeeded();
    }
  }

  if (cardSeenThisTick && !seenUid.isEmpty())
  {
    _cardPresent = true;
    _lastUid = seenUid;
    _removePending = false;
    _removePendingStartMs = 0;

    if (_lastEnableMs != 0 && (now - _lastEnableMs) < ENABLE_READ_CONFIRM_RESET_MS)
    {
      _pendingReadUid = seenUid;
      _pendingReadCount = 1;
      _pendingReadStartMs = now;
      return;
    }

    if (now < _bootWarmupUntilMs)
    {
      _pendingReadUid = seenUid;
      _pendingReadCount = READ_CONFIRM_COUNT;
      _pendingReadStartMs = now;
      return;
    }

    if (_publishedPresent)
    {
      _pendingReadUid = "";
      _pendingReadCount = 0;
      _pendingReadStartMs = 0;
      return;
    }

    if (seenUid != _pendingReadUid)
    {
      _pendingReadUid = seenUid;
      _pendingReadCount = 1;
      _pendingReadStartMs = now;
      return;
    }

    if (_pendingReadCount < 0xFF)
    {
      _pendingReadCount++;
    }

    const bool readConfirmed =
        _pendingReadCount >= READ_CONFIRM_COUNT &&
        (now - _pendingReadStartMs) >= READ_CONFIRM_MS;
    const bool readAllowed =
        !(_lastEventType == EVENT_REMOVE && (now - _lastEventMs) < EVENT_REFRACTORY_MS);

    if (readConfirmed && readAllowed)
    {
      _publishedPresent = true;
      _pendingReadUid = "";
      _pendingReadCount = 0;
      _pendingReadStartMs = 0;
      _lastEventType = EVENT_READ;
      _lastEventMs = now;
      if (_onRead)
      {
        _onRead(seenUid);
      }
    }
    return;
  }

  _pendingReadUid = "";
  _pendingReadCount = 0;
  _pendingReadStartMs = 0;

  if (!cardSeenThisTick && _cardPresent && _settings.miss >= 0)
  {
    if (now < _bootWarmupUntilMs || now < _suppressRemoveUntil || !_publishedPresent)
    {
      return;
    }

    const bool removeExpired =
        (_removeTimeMs == 0) || (_lastSeenMs > 0 && (now - _lastSeenMs) >= _removeTimeMs);
    if (!removeExpired)
    {
      return;
    }

    if (!_removePending)
    {
      _removePending = true;
      _removePendingStartMs = now;
      return;
    }

    const bool removeConfirmed = (now - _removePendingStartMs) >= REMOVE_PENDING_GRACE_MS;
    const bool removeAllowed =
        !(_lastEventType == EVENT_READ && (now - _lastEventMs) < EVENT_REFRACTORY_MS);

    if (removeConfirmed && removeAllowed)
    {
      _cardPresent = false;
      _publishedPresent = false;
      _removePending = false;
      _removePendingStartMs = 0;
      _lastEventType = EVENT_REMOVE;
      _lastEventMs = now;
      _lastUid = "";
      if (_onRemove)
      {
        _onRemove();
      }
    }
  }
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
