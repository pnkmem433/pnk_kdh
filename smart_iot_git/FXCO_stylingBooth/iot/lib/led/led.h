#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Led {
public:
  Led(uint8_t pin);
  void begin();                        // 핀 초기화 및 태스크 생성
  void on();                           // LED 켜기 (blink 중단)
  void off();                          // LED 끄기 (blink 중단)
  void blink(int onTime);              // on/off 동일 주기 점멸
  void blink(int onTime, int offTime); // on/off 다른 주기 점멸

private:
  static void ledTask(void *param);    // LED 제어용 FreeRTOS 태스크

  uint8_t _pin;
  TaskHandle_t _blinkTaskHandle;
  int _onTime;
  int _offTime;
  bool _isBlinking;
  bool _state;                         // on/off 상태 유지
};

#endif
