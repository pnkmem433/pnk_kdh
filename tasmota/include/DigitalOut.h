#pragma once

#include <Arduino.h>

class DigitalOut {
public:
  DigitalOut(uint8_t pin, bool activeHigh = true);

  void begin();
  void on();
  void off();
  void blink(unsigned long intervalMs);
  void blink(unsigned long onMs, unsigned long offMs);
  void stopBlink();
  void tick();
  bool isOn() const;

private:
  void writeRaw(bool logicalOn);

  uint8_t pin_;
  bool activeHigh_;
  bool state_ = false;
  bool blinking_ = false;
  bool blinkState_ = false;
  unsigned long onMs_ = 0;
  unsigned long offMs_ = 0;
  unsigned long lastToggleMs_ = 0;
};
