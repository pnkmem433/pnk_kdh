/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - 센서 이벤트 타입, 큐 메시지 구조, API 선언 정의.
*/
#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include <Arduino.h>
#include <vector>

enum class Event { Low, High };

constexpr int NO_DELAY = 0;

/*
  SensorEvent
  - event 상태가 delayMs 동안 유지되면 action 실행.
*/
struct SensorEvent {
  Event event;
  int delayMs;

  std::function<void()> action;
};

/*
  EventQueue
  - 센서 태스크에서 worker 태스크로 이벤트를 전달하기 위한 메시지.
  - 큐 분리 이유: 센서 감시 루프에서 콜백 실행으로 블로킹되지 않게 하기 위함.
*/
struct EventQueue {
  int sensorPin;
  Event event;
  uint32_t delayMs;
  std::function<void()> action;
};

class Sensor {
public:
  /*
    Sensor(pin)
    - 입력: 디지털 입력 핀 번호
    - 출력: 객체 생성
    - 부작용: 핀 번호/이벤트 목록 초기화
  */
  Sensor(int pin, uint8_t pinModeValue = INPUT);

  /*
    begin(events)
    - 입력: 이벤트 규칙 목록(event 상태 + delayMs + action)
    - 출력: 없음
    - 부작용:
      - pinMode(INPUT) 설정
      - 공용 이벤트 큐/워커 태스크 생성(최초 1회)
      - 센서 폴링 태스크 생성
    - 실패/주의:
      - 이벤트 action은 워커 컨텍스트에서 실행되므로 블로킹 로직을 길게 두면 큐 지연이 증가한다.
  */
  void begin(std::vector<SensorEvent> events);

private:
  int pin;
  uint8_t pinModeValue;
  std::vector<SensorEvent> events;

  static QueueHandle_t eventQueue;
  static void workerTask(void *param);
};

#endif

