/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - 센서 폴링/디바운스/이벤트 큐 워커 구현.
*/
#include "sensor.h"

QueueHandle_t Sensor::eventQueue = nullptr;

/*
  Sensor(pin)
  - 입력: 감시할 디지털 핀
  - 부작용: 핀 번호 저장
*/
Sensor::Sensor(int pin) : pin(pin) {}

/*
  begin(events)
  - 무엇: 센서 감시 태스크와 워커 큐를 구성해 이벤트 기반 콜백 실행 체계를 시작한다.
  - 동시성: 감시 태스크는 이벤트 감지/큐 적재, 워커 태스크는 action 실행 담당.
  - 주의: action 내부 블로킹이 길면 후속 이벤트 처리가 밀릴 수 있다.
*/
void Sensor::begin(std::vector<SensorEvent> events) {
  this->events = events;
  pinMode(pin, INPUT);

  if (eventQueue == nullptr) {
    // 큐 크기 100: 이벤트 순간 폭주에도 worker가 따라잡을 완충 용량
    eventQueue = xQueueCreate(100, sizeof(EventQueue));
    xTaskCreate(workerTask, "DoorWorkerTask", 4096, NULL, 1, NULL);
  }

  // 센서 태스크: 상태 변화 감시 + 지연 조건 평가 + 큐 전송
  xTaskCreate(
      [](void *arg) {
        Sensor *sensor = static_cast<Sensor *>(arg);

        Event lastEvent = digitalRead(sensor->pin) == HIGH
                                  ? Event::High
                                  : Event::Low;
        uint64_t lastTime = esp_timer_get_time();
        uint64_t lastDebounceTime = 0;

        std::vector<bool> isSend(sensor->events.size(), true);

        while (1) {
          Event currentEvent = digitalRead(sensor->pin) == HIGH
                                       ? Event::High
                                       : Event::Low;

          if (currentEvent != lastEvent) {
            uint64_t currentTime = esp_timer_get_time();
            // 10ms 디바운스: 기계식 접점 떨림 제거
            if (currentTime - lastDebounceTime > 10000) {
              lastDebounceTime = currentTime;
              lastEvent = currentEvent;
              lastTime = currentTime;
              std::fill(isSend.begin(), isSend.end(), false);
            }
          }

          for (size_t i = 0; i < sensor->events.size(); i++) {
            const auto &event = sensor->events[i];

            if (currentEvent == event.event) {
              if (esp_timer_get_time() - lastTime >= event.delayMs * 1000ULL) {
                if (!isSend[i]) {
                  isSend[i] = true;

                  EventQueue msg = {sensor->pin, event.event,
                                          event.delayMs, event.action};
                  xQueueSend(Sensor::eventQueue, &msg, portMAX_DELAY);
                }
              }
            }
          }

          vTaskDelay(pdMS_TO_TICKS(10));
        }
      },
      "nDoorSensorTask", 4096, this, 1, NULL);
}

/*
  workerTask(param)
  - 큐에서 이벤트를 꺼내 등록된 action을 실행한다.
*/
void Sensor::workerTask(void *param) {
  EventQueue msg;
  while (1) {
    if (xQueueReceive(eventQueue, &msg, portMAX_DELAY) == pdTRUE) {
      if (msg.action) {
        msg.action();
      }
    }
  }
}


