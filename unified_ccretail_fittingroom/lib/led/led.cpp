/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - LED 태스크 기반 점멸/고정 출력 구현.
*/
#include "led.h"

/*
  LED(pin)
  - 입력: LED 출력 핀 번호
  - 부작용: 내부 상태 초기화
*/
LED::LED(uint8_t pin)
    : _pin(pin), _blinkTaskHandle(NULL),
      _onTime(0), _offTime(0),
      _isBlinking(false), _state(false) {}

/*
  begin()
  - 출력 핀 초기화 후 제어 태스크를 생성한다.
  - 태스크 분리 이유: blink 타이밍을 메인 로직 지연과 분리하기 위해.
*/
void LED::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);

  // Core 1 고정: 통신 태스크와 분리해 점멸 타이밍 흔들림 완화
  xTaskCreatePinnedToCore(
      ledTask,
      "LedTask",
      2048,
      this,
      1,
      &_blinkTaskHandle,
      1);
}

/*
  on()
  - blink 모드를 해제하고 고정 ON 상태로 전환.
*/
void LED::on() {
  _isBlinking = false;
  _state = true;
}

/*
  off()
  - blink 모드를 해제하고 고정 OFF 상태로 전환.
*/
void LED::off() {
  _isBlinking = false;
  _state = false;
}

/*
  blink(onTime)
  - on/off 동일 주기로 점멸.
*/
void LED::blink(int onTime) {
  blink(onTime, onTime);
}

/*
  blink(onTime, offTime)
  - on/off 비대칭 주기로 점멸.
*/
void LED::blink(int onTime, int offTime) {
  _onTime = onTime;
  _offTime = offTime;
  _isBlinking = true;
}

/*
  ledTask(param)
  - LED 상태 플래그를 주기적으로 물리 핀에 반영한다.
*/
void LED::ledTask(void *param) {
  LED *led = static_cast<LED *>(param);

  for (;;) {
    if (led->_isBlinking) {
      digitalWrite(led->_pin, HIGH);
      vTaskDelay(pdMS_TO_TICKS(led->_onTime));
      if (!led->_isBlinking) continue;

      digitalWrite(led->_pin, LOW);
      vTaskDelay(pdMS_TO_TICKS(led->_offTime));
      if (!led->_isBlinking) continue;

    } else {
      digitalWrite(led->_pin, led->_state ? HIGH : LOW);
      // 10ms 폴링으로 상태 반영 지연 최소화 + CPU 점유율 억제
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}


