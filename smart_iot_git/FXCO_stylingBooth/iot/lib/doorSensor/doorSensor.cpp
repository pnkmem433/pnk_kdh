#include "DoorSensor.h"

QueueHandle_t DoorSensor::eventQueue = nullptr;

DoorSensor::DoorSensor(int pin) : pin(pin) {}

void DoorSensor::begin(std::vector<DoorSensorEvent> events) {
  this->events = events;
  pinMode(pin, INPUT);

  // Queue & Worker Task 최초 1회만 생성
  if (eventQueue == nullptr) {
    eventQueue = xQueueCreate(100, sizeof(DoorEventMessage));
    xTaskCreate(workerTask, "DoorWorkerTask", 4096, NULL, 1, NULL);
  }

  // 센서 모니터링 Task 생성
  xTaskCreate(
      [](void *arg) {
        DoorSensor *sensor = static_cast<DoorSensor *>(arg);

        DoorEvent lastEvent = digitalRead(sensor->pin) == HIGH
                                  ? DoorEvent::Closed
                                  : DoorEvent::Opened;
        uint64_t lastTime = esp_timer_get_time();
        uint64_t lastDebounceTime = 0; // 마지막 디바운스 체크 시간

        std::vector<bool> isSend(sensor->events.size(), true);

        while (1) {
          DoorEvent currentEvent = digitalRead(sensor->pin) == HIGH
                                       ? DoorEvent::Closed
                                       : DoorEvent::Opened;

          // 상태 변화가 감지되면, 디바운스 시간 동안 대기 후 실제 상태 갱신
          if (currentEvent != lastEvent) {
            uint64_t currentTime = esp_timer_get_time();
            // 10ms 디바운스 딜레이 하드코딩
            if (currentTime - lastDebounceTime > 10000) { // 10ms 딜레이
              // 디바운스 시간 지난 후에 상태 변경
              lastDebounceTime = currentTime;
              lastEvent = currentEvent;
              lastTime = currentTime;
              std::fill(isSend.begin(), isSend.end(), false);
            }
          }

          // 등록된 이벤트들 확인
          for (size_t i = 0; i < sensor->events.size(); i++) {
            const auto &event = sensor->events[i];

            if (currentEvent == event.event) {
              if (esp_timer_get_time() - lastTime >= event.delayMs * 1000ULL) {
                if (!isSend[i]) {
                  isSend[i] = true;

                  // Queue에 메시지 푸시
                  DoorEventMessage msg = {sensor->pin, event.event,
                                          event.delayMs, event.action};
                  xQueueSend(DoorSensor::eventQueue, &msg, portMAX_DELAY);
                }
              }
            }
          }

          vTaskDelay(pdMS_TO_TICKS(10));
        }
      },
      "nDoorSensorTask", 2048, this, 1, NULL);
}

void DoorSensor::workerTask(void *param) {
  DoorEventMessage msg;
  while (1) {
    if (xQueueReceive(eventQueue, &msg, portMAX_DELAY) == pdTRUE) {
      // 이벤트 콜백 실행
      if (msg.action) {
        Serial.printf("\n\n[queue]\tSensor Pin %d\tEvent: %s\tDelay: %d ms\t"
                      "Queue: %d/%d used\n",
                      msg.sensorPin,
                      msg.event == DoorEvent::Opened ? "Open" : "Close",
                      msg.delayMs, uxQueueMessagesWaiting(eventQueue),
                      uxQueueMessagesWaiting(eventQueue) +
                          uxQueueSpacesAvailable(eventQueue));

        msg.action();
      }
    }
  }
}
