/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - 상태 LED 제어 인터페이스 정의.
*/
#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
  LED
  - 목적: 상태 표시 LED를 ON/OFF 또는 비동기 점멸로 제어한다.
  - 내부 태스크 기반으로 동작해 다른 작업 지연의 영향을 최소화한다.
*/
class LED {
public:
  /*
    LED(pin)
    - 입력: LED 출력 핀
    - 출력: 객체 생성
    - 부작용: 내부 상태 플래그 초기화
  */
  LED(uint8_t pin);

  /*
    begin()
    - 입력/출력: 없음
    - 부작용: pinMode 설정 및 LED 제어 태스크 생성
    - 주의: begin() 이후 on/off/blink 호출이 물리 출력으로 반영된다.
  */
  void begin();

  /*
    on()/off()
    - 입력/출력: 없음
    - 부작용: blink 모드를 중단하고 고정 ON/OFF 상태로 전환
  */
  void on();
  void off();

  /*
    blink(onTime [,offTime])
    - 입력: 점등/소등 시간(ms)
    - 출력: 없음
    - 부작용: 비동기 점멸 모드 활성화
    - 주의: 매우 짧은 값은 CPU 점유율 증가 및 시각적 식별성 저하를 유발할 수 있다.
  */
  void blink(int onTime);
  void blink(int onTime, int offTime);

private:
  static void ledTask(void *param);

  uint8_t _pin;
  TaskHandle_t _blinkTaskHandle;
  int _onTime;
  int _offTime;
  bool _isBlinking;
  bool _state;
};

#endif


