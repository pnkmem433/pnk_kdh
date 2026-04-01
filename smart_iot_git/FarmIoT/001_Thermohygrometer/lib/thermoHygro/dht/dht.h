#ifndef DHT22_THERMO_HYGRO_SENSOR_H
#define DHT22_THERMO_HYGRO_SENSOR_H

#include "Arduino.h"
#include <DHT.h>


#include "../thermoHygroSensor.h"

class DhtThermoHygroSensor : public ThermoHygroSensor {

public:
  DhtThermoHygroSensor(uint8_t pin, uint8_t type);
  void begin() override;
  float readTemperature() override;
  float readHumidity() override;

private:
  DHT dht;
};

#endif
