#ifndef DIGITAL_SENSOR_H
#define DIGITAL_SENSOR_H

#include <Arduino.h>
#include <vector>
#include <atomic>
#include <functional>

constexpr uint64_t NO_DELAY = 0;
constexpr uint64_t DEFAULT_DEBOUNCE_TIME_US = 10000;
constexpr int DEFAULT_TASK_PRIORITY = 2;
constexpr int SENSOR_TASK_STACK_SIZE = 4096;
constexpr int WORKER_TASK_STACK_SIZE = 4096;
constexpr int DEFAULT_QUEUE_SIZE = 100;
constexpr int MUTEX_TIMEOUT_MS = 100;

enum class EventPriority {
  Low = 0,
  Normal = 1,
  High = 2,
  Critical = 3
};

struct SensorEvent {
  bool event;
  uint64_t delayMs;
  std::function<void()> action;
};

class Sensor {
public:
  enum class FlagUpdateMode {
    Immediate,
    Debounced
  };

  Sensor(int pin, FlagUpdateMode mode = FlagUpdateMode::Debounced, uint64_t debounceTimeUs = DEFAULT_DEBOUNCE_TIME_US);
  ~Sensor();

  Sensor(const Sensor&) = delete;
  Sensor& operator=(const Sensor&) = delete;

  Sensor(Sensor&& other) noexcept;
  Sensor& operator=(Sensor&& other) noexcept;

  void begin();
  void end();
  void listen(std::vector<SensorEvent> events);
  bool state() const;
  void clearEvents();

  bool hasBeenHigh() const;
  bool hasBeenLow() const;
  void resetHighFlag();
  void resetLowFlag();
  void resetAllFlags();

  bool isInitialized() const { return initialized; }
  int getPin() const { return pin; }

private:
  int pin;
  FlagUpdateMode flagUpdateMode;
  uint64_t debounceTimeUs;
  std::vector<SensorEvent> events;
  mutable SemaphoreHandle_t eventMutex;
  TaskHandle_t sensorTaskHandle;
  std::atomic<bool> highFlag{false};
  std::atomic<bool> lowFlag{false};
  std::atomic<bool> initialized{false};

  static void sensorTask(void* param);

  void cleanup();
  bool createTasks();
  void deleteTasks();
};

#endif
