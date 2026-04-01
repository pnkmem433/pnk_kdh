#include <Arduino.h>
#include "thermoHygroSensor.h"

namespace {

constexpr uint32_t kUsbBaudRate = 115200;
constexpr uint32_t kSensorBaudRate = 9600;
constexpr uint8_t kMax485ControlPin = D5;
constexpr int8_t kSensorRxPin = D6;
constexpr int8_t kSensorTxPin = D7;

ThermoHygroSensor *thermoHygroSensor =
    new XyMd03ThermoHygroSensor(Serial1, 1, kMax485ControlPin, kMax485ControlPin);

} // namespace

void setup() {
  Serial.begin(kUsbBaudRate);
  delay(2000);

  Serial.println("XY-MD03 sensor read start");

  Serial1.begin(kSensorBaudRate, SERIAL_8N1, kSensorRxPin, kSensorTxPin);
  thermoHygroSensor->begin();
}

void loop() {
  const float temp = thermoHygroSensor->readTemperature();
  const float humi = thermoHygroSensor->readHumidity();

  if (isnan(temp) || isnan(humi)) {
    Serial.println("Sensor read failed");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temp, 2);
    Serial.print(" C, Humidity: ");
    Serial.print(humi, 2);
    Serial.println(" %");
  }

  delay(2000);
}
