#include "ButtonSensor.h"

ButtonSensor::ButtonSensor(uint8_t pin, bool activeLow)
    : pin_(pin), activeLow_(activeLow) {}

void ButtonSensor::begin() {
  pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT);
  bool reading = digitalRead(pin_) == (activeLow_ ? LOW : HIGH);
  stablePressed_ = reading;
  lastReading_ = reading;
  lastChangeMs_ = millis();
}

void ButtonSensor::onPressed(std::function<void()> callback) {
  callback_ = std::move(callback);
}

void ButtonSensor::tick(unsigned long debounceMs) {
  bool reading = digitalRead(pin_) == (activeLow_ ? LOW : HIGH);
  unsigned long now = millis();

  if (reading != lastReading_) {
    lastReading_ = reading;
    lastChangeMs_ = now;
  }

  if (now - lastChangeMs_ < debounceMs) {
    return;
  }

  if (reading == stablePressed_) {
    return;
  }

  stablePressed_ = reading;
  if (stablePressed_ && callback_) {
    callback_();
  }
}
