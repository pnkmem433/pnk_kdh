#ifndef THERMO_HYGRO_SENSOR_H
#define THERMO_HYGRO_SENSOR_H

class ThermoHygroSensor {
public:
  virtual void begin() = 0;
  virtual float readTemperature() = 0;
  virtual float readHumidity() = 0;
  
};

#include "dht/dht.h"
#include "xyMd03/xyMd03.h"

#endif