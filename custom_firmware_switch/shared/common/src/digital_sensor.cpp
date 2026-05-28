#include "digital_sensor.h"

Sensor::Sensor(uint8_t pin, unsigned long debounceMs)
    : _pin(pin), _debounceMs(debounceMs), _lastChangeMs(0), _lastReading(HIGH),
      _stableState(HIGH) {}

void Sensor::begin() {
  pinMode(_pin, INPUT_PULLUP);
  _lastReading = digitalRead(_pin);
  _stableState = _lastReading;
}

void Sensor::onFalling(std::function<void()> callback) {
  _fallingCallback = callback;
}

void Sensor::update(unsigned long now) {
  const bool reading = digitalRead(_pin);

  if (reading != _lastReading) {
    _lastReading = reading;
    _lastChangeMs = now;
  }

  if ((now - _lastChangeMs) < _debounceMs) {
    return;
  }

  if (reading != _stableState) {
    const bool previousStableState = _stableState;
    _stableState = reading;

    if (previousStableState == HIGH && _stableState == LOW && _fallingCallback) {
      _fallingCallback();
    }
  }
}

bool Sensor::state() const { return _stableState == HIGH; }
