#ifndef DIGITAL_SENSOR_H
#define DIGITAL_SENSOR_H

#include <Arduino.h>
#include <functional>

class Sensor {
public:
  explicit Sensor(uint8_t pin, unsigned long debounceMs = 30);

  void begin();
  void update(unsigned long now);
  void onFalling(std::function<void()> callback);
  bool state() const;

private:
  uint8_t _pin;
  unsigned long _debounceMs;
  unsigned long _lastChangeMs;
  bool _lastReading;
  bool _stableState;
  std::function<void()> _fallingCallback;
};

#endif
