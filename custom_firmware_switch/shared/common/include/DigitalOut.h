#ifndef DIGITALOUT_H
#define DIGITALOUT_H

#include <Arduino.h>

class DigitalOut {
public:
  DigitalOut(uint8_t pin, bool activeLevel = true);

  void begin();
  void on();
  void off();
  void blink(unsigned long intervalMs);
  void blink(unsigned long onMs, unsigned long offMs);
  void update(unsigned long now);
  bool isOn() const;

private:
  void write(bool on);

  uint8_t _pin;
  bool _activeLevel;
  bool _state;
  bool _blinking;
  bool _blinkState;
  unsigned long _onMs;
  unsigned long _offMs;
  unsigned long _lastToggleMs;
};

#endif
