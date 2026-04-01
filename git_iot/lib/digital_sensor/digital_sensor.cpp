/*
  [파일 설명] lib/digital_sensor/digital_sensor.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "digital_sensor.h"
#include <exception>

Sensor::Sensor(int pin, FlagUpdateMode mode, uint64_t debounceTimeUs)
    : pin(pin), flagUpdateMode(mode), debounceTimeUs(debounceTimeUs),
      sensorTaskHandle(nullptr), eventMutex(nullptr) {
  // 실제 RTOS 리소스는 begin()에서 할당한다.
}

Sensor::~Sensor() { end(); }

Sensor::Sensor(Sensor &&other) noexcept
    : pin(other.pin), flagUpdateMode(other.flagUpdateMode),
      debounceTimeUs(other.debounceTimeUs), events(std::move(other.events)),
      sensorTaskHandle(other.sensorTaskHandle), eventMutex(other.eventMutex),
      highFlag(other.highFlag.load()), lowFlag(other.lowFlag.load()),
      initialized(other.initialized.load()) {
  // 이동 후 원본 객체는 안전한 비활성 상태로 만든다.
  other.sensorTaskHandle = nullptr;
  other.eventMutex = nullptr;
  other.initialized.store(false);
}

Sensor &Sensor::operator=(Sensor &&other) noexcept {
  if (this != &other) {
    end();

    pin = other.pin;
    flagUpdateMode = other.flagUpdateMode;
    debounceTimeUs = other.debounceTimeUs;
    events = std::move(other.events);
    sensorTaskHandle = other.sensorTaskHandle;
    eventMutex = other.eventMutex;
    highFlag = other.highFlag.load();
    lowFlag = other.lowFlag.load();
    initialized = other.initialized.load();

    other.sensorTaskHandle = nullptr;
    other.eventMutex = nullptr;
    other.initialized.store(false);
  }
  return *this;
}

/*
  begin()
  ------------------------------------------------------------------------------
  [역할]
  센서 모듈을 실제로 가동한다.

  [세부 동작]
  1) 중복 초기화 방지
  2) 핀 번호 유효성 확인
  3) 이벤트 뮤텍스 생성
  4) pinMode(INPUT) 설정
  5) 감시 태스크 생성
*/
void Sensor::begin() {
  if (initialized) {
    Serial.println("[WARNING] Sensor already initialized");
    return;
  }

  if (pin < 0) {
    Serial.printf("[ERROR] Invalid pin number: %d\n", pin);
    return;
  }

  if (eventMutex == nullptr) {
    // 이벤트 벡터 동시 접근 보호용 뮤텍스
    eventMutex = xSemaphoreCreateMutex();
    if (eventMutex == nullptr) {
      Serial.println("[ERROR] Failed to create event mutex");
      return;
    }
  }

  // 입력 핀 모드 설정
  pinMode(pin, INPUT);
  initialized = true;

  if (!createTasks()) {
    Serial.println("[ERROR] Failed to create tasks, cleaning up");
    cleanup();
    initialized = false;
    return;
  }

  Serial.printf("[INFO] Sensor initialized on pin %d\n", pin);
}

void Sensor::end() {
  if (!initialized) {
    return;
  }

  cleanup();
  initialized = false;

  Serial.printf("[INFO] Sensor on pin %d cleaned up\n", pin);
}

/*
  createTasks()
  ------------------------------------------------------------------------------
  [역할]
  센서 감시 태스크를 생성한다.

  [반환]
  - true : 태스크가 이미 있거나 생성 성공
  - false: 생성 실패
*/
bool Sensor::createTasks() {
  if (sensorTaskHandle == nullptr) {
    Serial.printf("[DEBUG] Creating sensor task for pin %d\n", pin);
    BaseType_t result =
        xTaskCreate(sensorTask, "SensorTask", SENSOR_TASK_STACK_SIZE, this,
                    DEFAULT_TASK_PRIORITY, &sensorTaskHandle);

    if (result == pdPASS) {
      Serial.printf("[DEBUG] Sensor task created successfully, handle: %p\n",
                    sensorTaskHandle);
      return true;
    }

    Serial.printf("[ERROR] Failed to create sensor task, result: %d\n", result);
    return false;
  }

  Serial.println("[DEBUG] Sensor task already exists");
  return true;
}

void Sensor::deleteTasks() {
  if (sensorTaskHandle != nullptr) {
    TaskHandle_t taskToDelete = sensorTaskHandle;
    sensorTaskHandle = nullptr;
    vTaskDelete(taskToDelete);

    // 태스크 정리 시간을 조금 확보한다.
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void Sensor::cleanup() {
  deleteTasks();

  if (eventMutex != nullptr) {
    vSemaphoreDelete(eventMutex);
    eventMutex = nullptr;
  }

  highFlag = false;
  lowFlag = false;
}

/*
  sensorTask()
  ------------------------------------------------------------------------------
  [핵심 루프]
  - 10ms 주기로 입력 샘플링
  - 상태 변화 시 디바운싱 검사
  - 조건 충족 이벤트를 별도 ActionTask로 실행

  [중요 설계]
  - 감시 루프는 절대 오래 블로킹하지 않는다.
  - 액션은 분리 태스크에서 실행해 감시 누락을 방지한다.
*/
void Sensor::sensorTask(void *param) {
  Sensor *sensor = static_cast<Sensor *>(param);
  if (!sensor) {
    vTaskDelete(nullptr);
    return;
  }

  // 직전 상태/시간
  // lastTime은 "상태가 마지막으로 확정된 시점"으로 지연 조건 계산에 사용
  bool lastEvent = digitalRead(sensor->pin) == HIGH;
  uint64_t lastTime = esp_timer_get_time();
  uint64_t lastDebounceTime = 0;

  // 각 이벤트가 이미 실행되었는지 표시
  std::vector<bool> isSend;

  while (sensor->initialized) {
    bool currentEvent = digitalRead(sensor->pin) == HIGH;
    uint64_t currentTime = esp_timer_get_time();

    // 타이머 래핑 보호
    if (currentTime < lastTime) {
      lastTime = currentTime;
      lastDebounceTime = currentTime;
    }

    // Immediate 모드는 매 루프 상태를 즉시 플래그 반영
    if (sensor->flagUpdateMode == Sensor::FlagUpdateMode::Immediate) {
      if (currentEvent) {
        sensor->highFlag = true;
      } else {
        sensor->lowFlag = true;
      }
    }

    // 상태가 변했으면 디바운싱 검사
    // debounceTimeUs보다 짧은 토글은 노이즈로 간주해 무시
    if (currentEvent != lastEvent) {
      if (currentTime - lastDebounceTime > sensor->debounceTimeUs) {
        lastDebounceTime = currentTime;
        lastEvent = currentEvent;
        lastTime = currentTime;

        // Debounced 모드에서는 여기서만 플래그 반영
        if (sensor->flagUpdateMode == Sensor::FlagUpdateMode::Debounced) {
          if (currentEvent) {
            sensor->highFlag = true;
          } else {
            sensor->lowFlag = true;
          }
        }

        // 상태 전이 시점마다 이벤트 실행 여부를 초기화
        // (같은 상태에서 이벤트가 여러 번 실행되지 않게 보장)
        if (xSemaphoreTake(sensor->eventMutex,
                           pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
          size_t currentEventsSize = sensor->events.size();
          isSend.assign(currentEventsSize, false);
          xSemaphoreGive(sensor->eventMutex);
        }
      }
    }

    // 등록된 이벤트 목록 평가
    if (sensor->eventMutex != nullptr) {
      if (xSemaphoreTake(sensor->eventMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) ==
          pdTRUE) {
        size_t eventsSize = sensor->events.size();

        // 이벤트 목록 크기와 isSend 크기가 일치할 때만 처리
        // listen()으로 목록이 교체된 순간의 불일치 상태를 방어한다.
        if (isSend.size() == eventsSize) {
          for (size_t i = 0; i < eventsSize; i++) {
            const auto &event = sensor->events[i];

            // 현재 핀 상태가 이벤트 조건과 같을 때만 시간 조건 확인
            if (currentEvent == event.event) {
              uint64_t timeDiff =
                  (currentTime >= lastTime)
                      ? (currentTime - lastTime)
                      : (UINT64_MAX - lastTime + currentTime + 1);

              // 지정 지연시간(delayMs) 이상 유지되면 액션 실행
              if (timeDiff >= event.delayMs * 1000ULL) {
                if (!isSend[i]) {
                  isSend[i] = true;

                  if (event.action) {
                    // 액션은 별도 태스크에서 실행해 센서 루프 블로킹 방지
                    auto *actionPtr = new std::function<void()>(event.action);
                    BaseType_t result = xTaskCreate(
                        [](void *param) {
                          auto action =
                              *static_cast<std::function<void()> *>(param);

                          Serial.printf(
                              "[ACTION] Executing action in separate task\n");

                          try {
                            action();
                          } catch (const std::exception &e) {
                            Serial.printf("[ERROR] Exception in action: %s\n",
                                          e.what());
                          } catch (...) {
                            Serial.println("[ERROR] Unknown exception in action");
                          }

                          delete static_cast<std::function<void()> *>(param);
                          vTaskDelete(nullptr);
                        },
                        "ActionTask", 2048, actionPtr, 2, nullptr);

                    if (result != pdPASS) {
                      Serial.printf(
                          "[ERROR] Failed to create action task, result: %d\n",
                          result);
                      delete actionPtr;
                    }
                  }
                }
              }
            }
          }
        }

        xSemaphoreGive(sensor->eventMutex);
      }
    }

    // 샘플링 주기: 10ms
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  vTaskDelete(nullptr);
}

/*
  listen(events)
  ------------------------------------------------------------------------------
  [역할]
  감시할 이벤트 목록을 등록/교체한다.

  [주의]
  - 기존 목록은 통째로 교체된다.
  - begin() 이전 호출은 무시된다.
*/
void Sensor::listen(std::vector<SensorEvent> events) {
  if (!initialized) {
    Serial.println("[ERROR] Sensor not initialized, call begin() first");
    return;
  }

  if (eventMutex == nullptr) {
    Serial.println("[ERROR] Event mutex not available");
    return;
  }

  if (xSemaphoreTake(eventMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // 등록 목록을 통째로 교체
    this->events = std::move(events);
    xSemaphoreGive(eventMutex);
    Serial.printf("[INFO] Registered %d events for pin %d\n", this->events.size(),
                  pin);
  } else {
    Serial.println("[ERROR] Failed to acquire event mutex in listen()");
  }
}

bool Sensor::state() const {
  if (!initialized) {
    Serial.println("[ERROR] Sensor not initialized, call begin() first");
    return false;
  }
  return digitalRead(pin) == HIGH;
}

/*
  clearEvents()
  ------------------------------------------------------------------------------
  [역할]
  현재 등록된 이벤트 목록을 모두 제거한다.
*/
void Sensor::clearEvents() {
  if (!initialized) {
    return;
  }

  if (eventMutex != nullptr) {
    if (xSemaphoreTake(eventMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      events.clear();
      xSemaphoreGive(eventMutex);
    } else {
      Serial.println("[ERROR] Failed to acquire mutex in clearEvents()");
    }
  }
}

bool Sensor::hasBeenHigh() const { return highFlag.load(); }
bool Sensor::hasBeenLow() const { return lowFlag.load(); }

void Sensor::resetHighFlag() { highFlag.store(false); }
void Sensor::resetLowFlag() { lowFlag.store(false); }
void Sensor::resetAllFlags() {
  highFlag.store(false);
  lowFlag.store(false);
}

