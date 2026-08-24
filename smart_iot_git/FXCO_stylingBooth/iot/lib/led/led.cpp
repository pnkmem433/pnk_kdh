#include "led.h"

Led::Led(uint8_t pin)
    : _pin(pin), _blinkTaskHandle(NULL),
      _onTime(0), _offTime(0),
      _isBlinking(false), _state(false) {}

void Led::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);

  // LED 제어 태스크 1회 생성
  xTaskCreatePinnedToCore(
      ledTask,        // 태스크 함수
      "LedTask",      // 태스크 이름
      2048,           // 스택 크기
      this,           // 매개변수
      1,              // 우선순위 낮게
      &_blinkTaskHandle,
      1);             // Core 1 고정 (Ethernet과 분리)
}

void Led::on() {
  _isBlinking = false;
  _state = true;      // 상태 저장 → 태스크에서 HIGH 유지
}

void Led::off() {
  _isBlinking = false;
  _state = false;     // 상태 저장 → 태스크에서 LOW 유지
}

// blink(int onTime) : on/off 동일 시간
void Led::blink(int onTime) {
  blink(onTime, onTime);
}

// blink(int onTime, int offTime) : 별도 지정
void Led::blink(int onTime, int offTime) {
  _onTime = onTime;
  _offTime = offTime;
  _isBlinking = true;
}

// LED 제어 태스크
void Led::ledTask(void *param) {
  Led *led = static_cast<Led *>(param);

  for (;;) {
    if (led->_isBlinking) {
      // 점멸 모드
      digitalWrite(led->_pin, HIGH);
      vTaskDelay(pdMS_TO_TICKS(led->_onTime));
      if (!led->_isBlinking) continue; // 중간에 on/off 전환되면 즉시 종료

      digitalWrite(led->_pin, LOW);
      vTaskDelay(pdMS_TO_TICKS(led->_offTime));
      if (!led->_isBlinking) continue;

    } else {
      // on/off 상태 유지
      digitalWrite(led->_pin, led->_state ? HIGH : LOW);
      vTaskDelay(pdMS_TO_TICKS(10)); // CPU 점유 최소화
    }
  }
}
