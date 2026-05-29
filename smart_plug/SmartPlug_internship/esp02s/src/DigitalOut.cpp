#include "DigitalOut.h"

DigitalOut::DigitalOut(uint8_t pin, bool activeLevel)
    : _pin(pin), _activeLevel(activeLevel), _state(false), _blinking(false),
      _blinkState(false), _onMs(0), _offMs(0), _lastToggleMs(0) {}

void DigitalOut::begin() {
  pinMode(_pin, OUTPUT);
  off();
}

void DigitalOut::write(bool on) {
  digitalWrite(_pin, on == _activeLevel ? HIGH : LOW);
}

void DigitalOut::on() {
  _blinking = false;
  _state = true;
  write(true);
}

void DigitalOut::off() {
  _blinking = false;
  _state = false;
  write(false);
}

void DigitalOut::blink(unsigned long intervalMs) { blink(intervalMs, intervalMs); }

void DigitalOut::blink(unsigned long onMs, unsigned long offMs) {
  _blinking = true;
  _onMs = onMs;
  _offMs = offMs;
}

void DigitalOut::update(unsigned long now) {
  if (!_blinking) {
    write(_state);
    return;
  }

  const unsigned long interval = _blinkState ? _onMs : _offMs;
  if (now - _lastToggleMs >= interval) {
    _blinkState = !_blinkState;
    _lastToggleMs = now;
    write(_blinkState);
  }
}

bool DigitalOut::isOn() const { return _state; }
