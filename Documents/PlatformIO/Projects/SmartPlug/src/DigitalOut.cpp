#include "DigitalOut.h"

DigitalOut::DigitalOut(uint8_t pin, bool activeLevel)
    : _pin(pin), _activeLevel(activeLevel), _taskHandle(NULL), _onTime(0),
      _offTime(0), _isBlinking(false), _state(false) {}

void DigitalOut::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, _activeLevel ? LOW : HIGH);
  xTaskCreatePinnedToCore(digitalTask, "DigitalOutTask", 2048, this, 1,
                          &_taskHandle, 1);
}

void DigitalOut::on() {
  _isBlinking = false;
  _state = true;
}

void DigitalOut::off() {
  _isBlinking = false;
  _state = false;
}

void DigitalOut::blink(int onTime) { blink(onTime, onTime); }

void DigitalOut::blink(int onTime, int offTime) {
  _onTime = onTime;
  _offTime = offTime;
  _isBlinking = true;
}

bool DigitalOut::isOn() { return _state; }

void DigitalOut::digitalTask(void *param) {
  DigitalOut *obj = static_cast<DigitalOut *>(param);

  for (;;) {
    if (obj->_isBlinking) {
      digitalWrite(obj->_pin, obj->_activeLevel ? HIGH : LOW);
      vTaskDelay(pdMS_TO_TICKS(obj->_onTime));
      if (!obj->_isBlinking)
        continue;

      digitalWrite(obj->_pin, obj->_activeLevel ? LOW : HIGH);
      vTaskDelay(pdMS_TO_TICKS(obj->_offTime));
      if (!obj->_isBlinking)
        continue;
    } else {
      digitalWrite(obj->_pin, obj->_state ? (obj->_activeLevel ? HIGH : LOW)
                                          : (obj->_activeLevel ? LOW : HIGH));
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}
