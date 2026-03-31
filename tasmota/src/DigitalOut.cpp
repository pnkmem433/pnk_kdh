#include "DigitalOut.h"

DigitalOut::DigitalOut(uint8_t pin, bool activeHigh)
    : pin_(pin), activeHigh_(activeHigh) {}

void DigitalOut::begin() {
  pinMode(pin_, OUTPUT);
  off();
}

void DigitalOut::on() {
  blinking_ = false;
  state_ = true;
  writeRaw(true);
}

void DigitalOut::off() {
  blinking_ = false;
  state_ = false;
  writeRaw(false);
}

void DigitalOut::blink(unsigned long intervalMs) { blink(intervalMs, intervalMs); }

void DigitalOut::blink(unsigned long onMs, unsigned long offMs) {
  blinking_ = true;
  onMs_ = onMs;
  offMs_ = offMs;
  blinkState_ = true;
  lastToggleMs_ = millis();
  writeRaw(true);
}

void DigitalOut::stopBlink() { blinking_ = false; }

void DigitalOut::tick() {
  if (!blinking_) {
    return;
  }

  unsigned long now = millis();
  unsigned long interval = blinkState_ ? onMs_ : offMs_;
  if (now - lastToggleMs_ < interval) {
    return;
  }

  blinkState_ = !blinkState_;
  lastToggleMs_ = now;
  writeRaw(blinkState_);
}

bool DigitalOut::isOn() const { return state_; }

void DigitalOut::writeRaw(bool logicalOn) {
  digitalWrite(pin_, logicalOn == activeHigh_ ? HIGH : LOW);
  if (!blinking_) {
    state_ = logicalOn;
  }
}
