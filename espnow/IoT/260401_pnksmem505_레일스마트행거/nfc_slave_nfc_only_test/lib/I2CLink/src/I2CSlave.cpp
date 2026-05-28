#include "I2CSlave.h"

I2CSlave* I2CSlave::_instance = nullptr;

I2CSlave::I2CSlave(const Config& config)
    : _config(config),
      _qHead(0),
      _qTail(0),
      _qCount(0),
      _qMax(QUEUE_MAX),
      _echoUpdated(false),
      _echoLen(0),
      _taskRunner(config.task),
      _onReceive(nullptr),
      _intervalMs(50),
      _startDelayMs(0) {
  if (_config.queueMax > 0 && _config.queueMax <= QUEUE_MAX) {
    _qMax = _config.queueMax;
  }
}

void I2CSlave::begin() {
  _instance = this;
  Wire.begin(_config.address, _config.sda, _config.scl, _config.frequency);
  Wire.onRequest(I2CSlave::onRequestThunk);
  Wire.onReceive(I2CSlave::onReceiveThunk);
}

void I2CSlave::begin(const Begin& beginConfig) {
  begin();

  _onReceive = beginConfig.onReceive;
  if (beginConfig.intervalMs > 0) {
    _intervalMs = beginConfig.intervalMs;
  }
  _startDelayMs = beginConfig.startDelayMs;

  if (!_taskRunner) {
    return;
  }
  if (!_onReceive) {
    return;
  }

  _taskRunner->begin({
      .loop = nullptr,                             // 컨텍스트 루프를 사용
      .loopWithContext = I2CSlave::taskLoopThunk,  // TaskRunner에서 호출할 루프 함수
      .context = this,                             // 루프 함수로 전달할 컨텍스트
  });
}

void I2CSlave::send(const Send& send) {
  String trimmed = send.value;
  if (trimmed.length() > VALUE_MAX) {
    trimmed = trimmed.substring(0, VALUE_MAX);
  }

  portENTER_CRITICAL(&_mux);
  if (_qCount >= _qMax) {
    // 큐가 가득 찼다면 가장 오래된 항목을 버리고 최신 항목을 넣음
    _qHead = (uint8_t)((_qHead + 1) % QUEUE_MAX);
    _qCount--;
  }

  QueueItem& slot = _queue[_qTail];
  slot.len = (uint8_t)trimmed.length();
  for (uint8_t i = 0; i < slot.len; i++) {
    slot.data[i] = (uint8_t)trimmed[i];
  }
  _qTail = (uint8_t)((_qTail + 1) % QUEUE_MAX);
  _qCount++;
  portEXIT_CRITICAL(&_mux);
}

void I2CSlave::onRequestThunk() {
  if (_instance) {
    _instance->onRequest();
  }
}

void I2CSlave::onReceiveThunk(int count) {
  if (_instance) {
    _instance->onReceive(count);
  }
}

void I2CSlave::onRequest() {
  uint8_t out[TX_MAX];
  uint8_t outLen = 0;
  uint8_t sentCount = 0;

  out[outLen++] = 0;  // 보낼 값 개수(나중에 채움)

  portENTER_CRITICAL_ISR(&_mux);
  uint8_t count = _qCount;

  uint8_t idx = _qHead;
  while (sentCount < count) {
    QueueItem& item = _queue[idx];
    uint8_t len = item.len;
    uint8_t need = 1 + len + 2;  // len + data + crc16
    if (outLen + need > TX_MAX) {
      break;
    }
    out[outLen++] = len;
    for (uint8_t i = 0; i < len; i++) {
      out[outLen++] = item.data[i];
    }
    uint16_t crc = crc16ccitt(item.data, len);
    out[outLen++] = (uint8_t)(crc & 0xFF);
    out[outLen++] = (uint8_t)((crc >> 8) & 0xFF);

    sentCount++;
    idx = (uint8_t)((idx + 1) % QUEUE_MAX);
  }

  if (sentCount > 0) {
    _qHead = (uint8_t)((_qHead + sentCount) % QUEUE_MAX);
    _qCount = (uint8_t)(_qCount - sentCount);
  }
  portEXIT_CRITICAL_ISR(&_mux);

  out[0] = sentCount;  // 실제로 보낸 값 개수
  Wire.write(out, outLen);
}

void I2CSlave::onReceive(int count) {
  if (count < 4) {
    while (Wire.available()) {
      Wire.read();
    }
    return;
  }

  uint8_t cmd = Wire.read();
  if (cmd != CMD_ECHO) {
    while (Wire.available()) {
      Wire.read();
    }
    return;
  }

  uint8_t len = Wire.read();
  uint8_t toRead = len;
  uint16_t crc = 0xFFFF;
  uint8_t temp[VALUE_MAX];
  uint8_t tempLen = 0;

  uint8_t i = 0;
  while (Wire.available() && i < toRead) {
    uint8_t b = (uint8_t)Wire.read();
    if (i < VALUE_MAX) {
      temp[tempLen++] = b;
    }
    // CRC 계산은 원본 길이를 기준으로 수행
    crc ^= (uint16_t)b << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    i++;
  }

  if (Wire.available() < 2) {
    while (Wire.available()) {
      Wire.read();
    }
    return;
  }

  uint16_t recvCrc = (uint16_t)Wire.read();
  recvCrc |= (uint16_t)Wire.read() << 8;

  if (i != toRead || len > VALUE_MAX || crc != recvCrc) {
    while (Wire.available()) {
      Wire.read();
    }
    return;
  }

  portENTER_CRITICAL_ISR(&_mux);
  _echoLen = tempLen;
  for (uint8_t k = 0; k < tempLen; k++) {
    _echoData[k] = temp[k];
  }
  _echoUpdated = true;
  portEXIT_CRITICAL_ISR(&_mux);

  while (Wire.available()) {
    Wire.read();
  }
}

bool I2CSlave::takeEchoString(String &out) {
  if (!_echoUpdated) {
    return false;
  }

  uint8_t data[VALUE_MAX];
  uint8_t len = 0;

  portENTER_CRITICAL(&_mux);
  len = _echoLen;
  for (uint8_t i = 0; i < len; i++) {
    data[i] = _echoData[i];
  }
  _echoUpdated = false;
  portEXIT_CRITICAL(&_mux);

  out.reserve(len);
  for (uint8_t i = 0; i < len; i++) {
    out += (char)data[i];
  }
  return true;
}

void I2CSlave::scanEcho(const Begin& scan) {
  if (!scan.onReceive) {
    return;
  }
  String echoStr;
  if (!takeEchoString(echoStr)) {
    return;
  }
  scan.onReceive(echoStr);
}

void I2CSlave::taskLoopThunk(void* context) { static_cast<I2CSlave*>(context)->taskLoop(); }

void I2CSlave::taskLoop() {
  if (_startDelayMs > 0) {
    delay(_startDelayMs);
    _startDelayMs = 0;
  }

  scanEcho({
      .onReceive = _onReceive,    // 에코 수신 처리 함수
      .intervalMs = _intervalMs,  // 스캔 주기(ms)
      .startDelayMs = 0,          // 시작 지연은 이미 처리
  });
  delay(_intervalMs);
}

uint16_t I2CSlave::crc16ccitt(const uint8_t* data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
  }
  return crc;
}
