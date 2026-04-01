#include "dht.h"

DhtThermoHygroSensor::DhtThermoHygroSensor(uint8_t pin, uint8_t type)
    : dht(pin, type) {}

void DhtThermoHygroSensor::begin() { dht.begin(); }

float DhtThermoHygroSensor::readTemperature() { return dht.readTemperature(); }

float DhtThermoHygroSensor::readHumidity() { return dht.readHumidity(); }