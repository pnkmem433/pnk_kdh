#include "digital_sensor.h"

Sensor::Sensor(int pin, FlagUpdateMode mode, uint64_t debounceTimeUs)
    : pin(pin), flagUpdateMode(mode), debounceTimeUs(debounceTimeUs),
      sensorTaskHandle(nullptr), eventMutex(nullptr) {
}

Sensor::~Sensor() { end(); }

Sensor::Sensor(Sensor &&other) noexcept
    : pin(other.pin), flagUpdateMode(other.flagUpdateMode),
      debounceTimeUs(other.debounceTimeUs), events(std::move(other.events)),
      sensorTaskHandle(other.sensorTaskHandle), eventMutex(other.eventMutex),
      highFlag(other.highFlag.load()), lowFlag(other.lowFlag.load()),
      initialized(other.initialized.load()) {
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
    eventMutex = xSemaphoreCreateMutex();
    if (eventMutex == nullptr) {
      Serial.println("[ERROR] Failed to create event mutex");
      return;
    }
  }

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
    } else {
      Serial.printf("[ERROR] Failed to create sensor task, result: %d\n",
                    result);
      return false;
    }
  } else {
    Serial.println("[DEBUG] Sensor task already exists");
    return true;
  }
}

void Sensor::deleteTasks() {
  if (sensorTaskHandle != nullptr) {
    TaskHandle_t taskToDelete = sensorTaskHandle;
    sensorTaskHandle = nullptr;
    vTaskDelete(taskToDelete);
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

void Sensor::sensorTask(void *param) {
  Sensor *sensor = static_cast<Sensor *>(param);
  if (!sensor) {
    Serial.println("[SENSOR_TASK] ERROR: null sensor pointer");
    vTaskDelete(nullptr);
    return;
  }

  Serial.printf("[SENSOR_TASK] Started for pin %d\n", sensor->pin);
  Serial.printf("[SENSOR_TASK] Free heap: %d\n", ESP.getFreeHeap());

  bool lastEvent = digitalRead(sensor->pin) == HIGH;
  uint64_t lastTime = esp_timer_get_time();
  uint64_t lastDebounceTime = 0;

  std::vector<bool> isSend;
  size_t lastEventsSize = 0;

  Serial.printf("[SENSOR_TASK] Initial pin state: %s\n", lastEvent ? "HIGH" : "LOW");

  while (sensor->initialized) {
    bool currentEvent = digitalRead(sensor->pin) == HIGH;
    uint64_t currentTime = esp_timer_get_time();

    if (currentTime < lastTime) {
      lastTime = currentTime;
      lastDebounceTime = currentTime;
    }

    if (sensor->flagUpdateMode == Sensor::FlagUpdateMode::Immediate) {
      if (currentEvent) {
        sensor->highFlag = true;
      } else {
        sensor->lowFlag = true;
      }
    }

    if (currentEvent != lastEvent) {
      if (currentTime - lastDebounceTime > sensor->debounceTimeUs) {
        lastDebounceTime = currentTime;
        lastEvent = currentEvent;
        lastTime = currentTime;

        if (sensor->flagUpdateMode == Sensor::FlagUpdateMode::Debounced) {
          if (currentEvent) {
            sensor->highFlag = true;
          } else {
            sensor->lowFlag = true;
          }
        }

        if (xSemaphoreTake(sensor->eventMutex,
                           pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
          size_t currentEventsSize = sensor->events.size();
          isSend.assign(currentEventsSize, false);
          lastEventsSize = currentEventsSize;
          xSemaphoreGive(sensor->eventMutex);
        }
      }
    }

    if (sensor->eventMutex != nullptr) {
      if (xSemaphoreTake(sensor->eventMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) ==
          pdTRUE) {
        size_t eventsSize = sensor->events.size();

        if (isSend.size() == eventsSize) {
          for (size_t i = 0; i < eventsSize; i++) {
            const auto &event = sensor->events[i];
            if (currentEvent == event.event) {
              uint64_t timeDiff =
                  (currentTime >= lastTime)
                      ? (currentTime - lastTime)
                      : (UINT64_MAX - lastTime + currentTime + 1);

              if (timeDiff >= event.delayMs * 1000ULL) {
                if (!isSend[i]) {
                  isSend[i] = true;

                  if (event.action) {
                    Serial.printf("[SENSOR_TASK] Triggering action for event %d, Free heap: %d\n", i, ESP.getFreeHeap());
                    auto *actionPtr = new std::function<void()>(event.action);
                    BaseType_t result = xTaskCreate(
                        [](void *param) {
                          auto action =
                              *static_cast<std::function<void()> *>(param);

                          Serial.printf("[ACTION] Executing action in separate task\n");
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
                        "ActionTask", 4096, actionPtr, 2, nullptr);

                    if (result != pdPASS) {
                      Serial.printf("[ERROR] Failed to create action task, result: %d\n",
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

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  vTaskDelete(nullptr);
}

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
    this->events = std::move(events);
    xSemaphoreGive(eventMutex);
    Serial.printf("[INFO] Registered %d events for pin %d\n",
                  this->events.size(), pin);
  } else {
    Serial.println("[ERROR] Failed to acquire event mutex in listen()");
    return;
  }
}

bool Sensor::state() const {
  if (!initialized) {
    Serial.println("[ERROR] Sensor not initialized, call begin() first");
    return false;
  }
  return digitalRead(pin) == HIGH;
}

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
