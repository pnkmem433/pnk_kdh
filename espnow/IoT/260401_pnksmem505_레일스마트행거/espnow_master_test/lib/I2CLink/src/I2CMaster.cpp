#include "I2CMaster.h"

I2CMaster::I2CMaster(const Config &config)
    : _config(config),
      _slaves({0}),
      _slaveCount(0),
      _taskRunner(config.task),
      _onDataReceive(nullptr),
      _intervalMs(500),
      _startDelayMs(0) {
  for (uint8_t addr : _config.slaves) {
    if (_slaveCount >= MAX_SLAVES) {
      break;
    }
    _slaves[_slaveCount++] = addr;
  }
}

void I2CMaster::begin() {
  Wire.begin(_config.sda, _config.scl, _config.frequency);
}

void I2CMaster::begin(const Begin &beginConfig) {
  begin();

  _onDataReceive = beginConfig.onDataReceive;
  if (beginConfig.intervalMs > 0) {
    _intervalMs = beginConfig.intervalMs;
  }
  _startDelayMs = beginConfig.startDelayMs;

  if (!_taskRunner) {
    return;
  }
  if (!_onDataReceive) {
    return;
  }

  _taskRunner->begin({
    .loop = nullptr, // 컨텍스트 루프를 사용
    .loopWithContext = I2CMaster::taskLoopThunk, // TaskRunner에서 호출할 루프 함수
    .context = this, // 루프 함수로 전달할 컨텍스트
  });
}

size_t I2CMaster::slaveCount() const {
  return _slaveCount;
}

I2CMaster::SlaveRange I2CMaster::slaves() const {
  return SlaveRange{ _slaves.data(), _slaveCount };
}

bool I2CMaster::requestValueRaw(uint8_t address, uint8_t *outValue, uint8_t &outLen) {
  uint8_t got = Wire.requestFrom(address, RX_MAX);
  if (got < 4) {
    return false;
  }

  uint8_t count = Wire.read();
  if (count == 0) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  if (!Wire.available()) {
    return false;
  }

  uint8_t len = Wire.read();
  if (len > VALUE_MAX) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  if (Wire.available() < (uint8_t)(len + 2)) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  outLen = len;
  for (uint8_t i = 0; i < len; i++) {
    outValue[i] = (uint8_t)Wire.read();
  }
  uint16_t recvCrc = (uint16_t)Wire.read();
  recvCrc |= (uint16_t)Wire.read() << 8;
  uint16_t calcCrc = crc16ccitt(outValue, len);
  if (recvCrc != calcCrc) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  while (Wire.available()) {
    Wire.read();
  }
  return true;
}

String I2CMaster::requestValueString(const RequestValueString &request) {
  uint8_t value[VALUE_MAX];
  uint8_t len = 0;
  if (!requestValueRaw(request.address, value, len)) {
    return String();
  }

  String out;
  out.reserve(len);
  for (uint8_t i = 0; i < len; i++) {
    out += (char)value[i];
  }
  return out;
}

void I2CMaster::sendEchoRaw(uint8_t address, const uint8_t *value, uint8_t len) {
  if (len > VALUE_MAX) {
    len = VALUE_MAX;
  }
  Wire.beginTransmission(address);
  Wire.write(CMD_ECHO);
  Wire.write(len);
  if (len > 0) {
    Wire.write(value, len);
  }
  uint16_t crc = crc16ccitt(value, len);
  Wire.write((uint8_t)(crc & 0xFF));
  Wire.write((uint8_t)((crc >> 8) & 0xFF));
  Wire.endTransmission();
}

void I2CMaster::sendEchoString(const SendEchoString &echo) {
  uint8_t len = (uint8_t)min((size_t)VALUE_MAX, echo.value.length());
  sendEchoRaw(echo.address, (const uint8_t *)echo.value.c_str(), len);
}

void I2CMaster::scanSlaves(const SlaveScan &scan) {
  if (!scan.onDataReceive) {
    return;
  }
  for (uint8_t addr : slaves()) {
    if (!addr) {
      continue;
    }

    uint8_t got = Wire.requestFrom(addr, RX_MAX);
    if (got < 2) {
      continue;
    }

    uint8_t count = Wire.read();
    if (count == 0) {
      while (Wire.available()) {
        Wire.read();
      }
      continue;
    }

    for (uint8_t item = 0; item < count; item++) {
      if (!Wire.available()) {
        break;
      }
      uint8_t len = Wire.read();
      if (len > VALUE_MAX) {
        break;
      }
      if (Wire.available() < (uint8_t)(len + 2)) {
        break;
      }

      String value;
      value.reserve(len);
      for (uint8_t i = 0; i < len; i++) {
        value += (char)Wire.read();
      }
      uint16_t recvCrc = (uint16_t)Wire.read();
      recvCrc |= (uint16_t)Wire.read() << 8;
      uint16_t calcCrc = crc16ccitt((const uint8_t *)value.c_str(), len);
      if (recvCrc != calcCrc) {
        break;
      }

      String echoStr = scan.onDataReceive({
        .address = addr,
        .value = value,
      });
      if (echoStr.length() == 0) {
        continue;
      }
      sendEchoString({
        .address = addr,
        .value = echoStr,
      });
    }

    while (Wire.available()) {
      Wire.read();
    }
  }
}

void I2CMaster::taskLoopThunk(void *context) {
  static_cast<I2CMaster *>(context)->taskLoop();
}

void I2CMaster::taskLoop() {
  if (_startDelayMs > 0) {
    delay(_startDelayMs);
    _startDelayMs = 0;
  }

  scanSlaves({
    .onDataReceive = _onDataReceive,
  });
  delay(_intervalMs);
}

uint16_t I2CMaster::crc16ccitt(const uint8_t *data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
  }
  return crc;
}
