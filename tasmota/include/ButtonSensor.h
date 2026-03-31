#pragma once

#include <Arduino.h>
#include <functional>

class ButtonSensor {
public:
  explicit ButtonSensor(uint8_t pin, bool activeLow = true);

  void begin();
  void onPressed(std::function<void()> callback);
  void tick(unsigned long debounceMs);

private:
  uint8_t pin_;
  bool activeLow_;
  bool stablePressed_ = false;
  bool lastReading_ = false;
  unsigned long lastChangeMs_ = 0;
  std::function<void()> callback_;
};
