#include "xyMd03.h"

XyMd03ThermoHygroSensor::XyMd03ThermoHygroSensor(Stream &serial,
                                                 uint8_t slave_address,
                                                 uint8_t pin_re, uint8_t pin_de)
    : _serial(serial), _slaveId(slave_address), _rePin(pin_re), _dePin(pin_de) {
}

void XyMd03ThermoHygroSensor::begin() {
  pinMode(_dePin, OUTPUT);
  pinMode(_rePin, OUTPUT);
  digitalWrite(_dePin, LOW);
  digitalWrite(_rePin, LOW);

  xMutex = xSemaphoreCreateMutex(); // 뮤텍스 초기화
}

float XyMd03ThermoHygroSensor::readTemperature() {
  auto result = read(0x0001, 1);
  if (result.size() == 0)
    return NAN; // 실패 시 안전하게 반환
  return result[0] / 10.0;
}

float XyMd03ThermoHygroSensor::readHumidity() {
  auto result = read(0x0002, 1);
  if (result.size() == 0)
    return NAN;
  return result[0] / 10.0;
}

uint16_t XyMd03ThermoHygroSensor::calculateCRC(uint8_t *data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}

std::vector<int16_t> XyMd03ThermoHygroSensor::read(uint16_t startAddr,
                                                   uint16_t numRegs) {
  FS_LOCK();

  std::vector<int16_t> result;

  uint8_t request[8];
  request[0] = _slaveId;
  request[1] = 0x04;
  request[2] = highByte(startAddr);
  request[3] = lowByte(startAddr);
  request[4] = highByte(numRegs);
  request[5] = lowByte(numRegs);

  uint16_t crc = calculateCRC(request, 6);
  request[6] = lowByte(crc);
  request[7] = highByte(crc);

  digitalWrite(_rePin, LOW);
  digitalWrite(_dePin, HIGH);
  delayMicroseconds(100);

  _serial.write(request, 8);
  _serial.flush();

  delayMicroseconds(100);
  digitalWrite(_dePin, LOW);
  digitalWrite(_rePin, LOW);

  uint8_t responseLen = 5 + 2 * numRegs;
  uint8_t response[responseLen];
  uint8_t index = 0;

  unsigned long startTime = millis();
  while ((millis() - startTime < 1000) && index < responseLen) {
    if (_serial.available()) {
      response[index++] = _serial.read();
    }
  }

  if (index != responseLen || response[1] != 0x04) {
    Serial.println("[Modbus] ERROR: Invalid response or timeout.");
    FS_UNLOCK();
    return result;
  }

  uint16_t crcResp =
      (response[responseLen - 1] << 8) | response[responseLen - 2];
  if (calculateCRC(response, responseLen - 2) != crcResp) {
    Serial.println("[Modbus] ERROR: CRC mismatch.");
    FS_UNLOCK();
    return result;
  }

  for (uint8_t i = 0; i < numRegs; i++) {
    uint16_t raw = (response[3 + i * 2] << 8) | response[4 + i * 2];

    int16_t value = static_cast<int16_t>(raw);
    result.push_back(value);
  }

  FS_UNLOCK();
  return result;
}
