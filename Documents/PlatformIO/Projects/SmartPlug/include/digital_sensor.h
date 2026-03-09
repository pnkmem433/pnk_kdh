#ifndef DIGITAL_SENSOR_H
#define DIGITAL_SENSOR_H

#include <Arduino.h>
#include <vector>
#include <atomic>
#include <functional>

// 상수 정의
constexpr uint64_t NO_DELAY = 0;
constexpr uint64_t DEFAULT_DEBOUNCE_TIME_US = 10000;  // 10ms
constexpr int DEFAULT_TASK_PRIORITY = 2;
constexpr int SENSOR_TASK_STACK_SIZE = 4096;
constexpr int WORKER_TASK_STACK_SIZE = 4096;
constexpr int DEFAULT_QUEUE_SIZE = 100;
constexpr int MUTEX_TIMEOUT_MS = 100;  // 뮤텍스 타임아웃 (100ms)

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
    Immediate,    // 매 루프마다 즉시 플래그 갱신
    Debounced     // 디바운스 후에 플래그 갱신
  };

  // 생성자 및 소멸자
  Sensor(int pin, FlagUpdateMode mode = FlagUpdateMode::Debounced, uint64_t debounceTimeUs = DEFAULT_DEBOUNCE_TIME_US);
  ~Sensor();

  // 복사 생성자 및 대입 연산자 삭제
  Sensor(const Sensor&) = delete;
  Sensor& operator=(const Sensor&) = delete;

  // 이동 생성자 및 대입 연산자
  Sensor(Sensor&& other) noexcept;
  Sensor& operator=(Sensor&& other) noexcept;

  // 공개 메서드
  void begin();
  void end();  // 리소스 정리
  void listen(std::vector<SensorEvent> events);
  bool state() const;  // 현재 핀 상태 읽기
  void clearEvents();

  // 플래그 접근 (스레드 안전)
  bool hasBeenHigh() const;
  bool hasBeenLow() const;
  void resetHighFlag();
  void resetLowFlag();
  void resetAllFlags();

  // 상태 확인
  bool isInitialized() const { return initialized; }
  int getPin() const { return pin; }

private:
  // 핀 및 설정
  int pin;
  FlagUpdateMode flagUpdateMode;
  uint64_t debounceTimeUs;

  // 이벤트 관리
  std::vector<SensorEvent> events;
  mutable SemaphoreHandle_t eventMutex;  // 이벤트 벡터 보호

  // FreeRTOS 리소스
  TaskHandle_t sensorTaskHandle;

  // 플래그 (원자적 연산)
  std::atomic<bool> highFlag{false};
  std::atomic<bool> lowFlag{false};

  // 초기화 상태
  std::atomic<bool> initialized{false};

  // 정적 메서드
  static void sensorTask(void* param);

  // 내부 메서드
  void cleanup();
  bool createTasks();
  void deleteTasks();
};


#endif // DIGITAL_SENSOR_H