#ifndef DIGITALOUT_H
#define DIGITALOUT_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class DigitalOut {
public:
  DigitalOut(uint8_t pin, bool activeLevel = true);
  void begin();
  void on();
  void off();
  void blink(int onTime);
  void blink(int onTime, int offTime);

  bool isOn();

private:
  static void digitalTask(void *param);

  uint8_t _pin;
  bool _activeLevel;
  TaskHandle_t _taskHandle;
  int _onTime;
  int _offTime;
  bool _isBlinking;
  bool _state;
};

#endif
