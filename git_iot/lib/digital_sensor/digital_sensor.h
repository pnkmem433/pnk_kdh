#ifndef DIGITAL_SENSOR_H
#define DIGITAL_SENSOR_H

#include <Arduino.h>
#include <atomic>
#include <functional>
#include <vector>

/*
  ==================================================================================
  digital_sensor 모듈 상세 설명
  ==================================================================================
  [목표]
  단순 digitalRead()를 넘어서,
  - 디바운싱
  - 상태 지속시간(delayMs) 조건
  - 이벤트 액션 비동기 실행
  - HIGH/LOW 플래그 기록
  까지 제공하는 고수준 입력 감시기.

  [핵심 아이디어]
  센서 감시 루프는 아주 짧고 안정적으로 유지하고,
  액션은 별도 태스크에서 실행해 감시 루프를 절대 막지 않는다.
  ==================================================================================
*/

constexpr uint64_t NO_DELAY = 0;
constexpr uint64_t DEFAULT_DEBOUNCE_TIME_US = 10000; // 10ms
constexpr int DEFAULT_TASK_PRIORITY = 2;
constexpr int SENSOR_TASK_STACK_SIZE = 2048;
constexpr int WORKER_TASK_STACK_SIZE = 4096;
constexpr int DEFAULT_QUEUE_SIZE = 100;
constexpr int MUTEX_TIMEOUT_MS = INT_MAX;

enum class EventPriority { Low = 0, Normal = 1, High = 2, Critical = 3 };

struct SensorEvent {
  // true(HIGH) 또는 false(LOW) 상태에서 트리거할지 지정
  bool event;

  // 해당 상태가 delayMs 이상 유지되었을 때 실행
  uint64_t delayMs;

  // 조건 충족 시 실행할 사용자 함수
  std::function<void()> action;
};

class Sensor {
public:
  enum class FlagUpdateMode {
    // 매 루프에서 현재 입력 상태를 즉시 플래그에 반영
    Immediate,

    // 디바운싱 통과한 상태 변화만 플래그 반영
    Debounced
  };

  Sensor(int pin, FlagUpdateMode mode = FlagUpdateMode::Debounced,
         uint64_t debounceTimeUs = DEFAULT_DEBOUNCE_TIME_US);
  ~Sensor();

  // 복사 금지
  Sensor(const Sensor &) = delete;
  Sensor &operator=(const Sensor &) = delete;

  // 이동 허용
  Sensor(Sensor &&other) noexcept;
  Sensor &operator=(Sensor &&other) noexcept;

  // 센서 시작/종료
  void begin();
  void end();

  // 이벤트 목록 등록(기존 목록 교체)
  void listen(std::vector<SensorEvent> events);

  // 현재 핀 상태 읽기
  bool state() const;

  // 이벤트 목록 비우기
  void clearEvents();

  // HIGH/LOW 감지 플래그 조회/리셋
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

  static void sensorTask(void *param);

  void cleanup();
  bool createTasks();
  void deleteTasks();
};

#endif // DIGITAL_SENSOR_H
