#ifndef XY_MD03_THERMO_HYGRO_SENSOR_H
#define XY_MD03_THERMO_HYGRO_SENSOR_H

#include "Arduino.h"
#include "vector"

#include "../thermoHygroSensor.h"

class XyMd03ThermoHygroSensor : public ThermoHygroSensor {

public:
  XyMd03ThermoHygroSensor(Stream &serial, uint8_t slave_address,
                          uint8_t pin_re = 0, uint8_t pin_de = 0);

  void begin() override;

  float readTemperature() override;
  float readHumidity() override;

private:
  Stream &_serial;
  uint8_t _dePin;
  uint8_t _rePin;
  uint8_t _slaveId;

  uint16_t calculateCRC(uint8_t *data, uint8_t length);
  std::vector<int16_t> read(uint16_t startAddr, uint16_t numRegs);
  SemaphoreHandle_t xMutex;

#define FS_LOCK()                                                              \
  xSemaphoreTake(xMutex, portMAX_DELAY);                                       \
  delay(100);

#define FS_UNLOCK()                                                            \
  delay(100);                                                                  \
  xSemaphoreGive(xMutex);
};

#endif
