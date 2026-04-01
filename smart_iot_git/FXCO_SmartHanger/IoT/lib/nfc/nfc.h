#ifndef NFC_H
#define NFC_H

#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <functional>

/*
  ==================================================================================
  Nfc 모듈 상세 설명
  ==================================================================================
  [왜 별도 모듈인가]
  NFC는 "계속 감시"가 핵심이다.
  앱 로직(HTTP/MQTT/UI)과 한 스레드에서 돌리면 작은 지연에도 감지 누락이 생긴다.

  [아키텍처]
  - readTask: RC522를 짧은 주기로 폴링해 이벤트를 큐에 넣음
  - callbackTask: 큐에서 이벤트를 꺼내 사용자 콜백 실행
  => I/O와 비즈니스 로직을 분리해 안정성 확보

  [중요한 상태값]
  - preNfcCard: 마지막으로 확정된 UID
  - isCardPresent: 카드 존재 여부
  - igoneTime: 순간 미검출을 제거하기 위한 완충 카운터(노이즈 흡수)

  [SPI 충돌 대응]
  TFT와 RC522가 SPI를 공유하는 환경을 고려해 spiMutex로 보호한다.
  ==================================================================================
*/

struct NfcActions {
  // 새로운 카드 UID가 감지됐을 때 호출된다.
  // 주의: 같은 카드가 계속 붙어 있는 동안에는 중복 호출되지 않도록 설계됨.
  std::function<void(String)> onScan;

  // 카드 제거가 확정되면 호출된다.
  std::function<void()> onRemove;

  // 제거 직전 preRemove 단계에서 호출된다.
  // 통신이 불안정할 때 restartCommunication() 같은 복구 로직을 넣는 훅.
  std::function<void()> onPrepareForRemoval;
};

struct NfcPinConfig {
  int ssPin;
  int rstPin;
};

enum class NfcEventType { Scan, Remove, preRemove };

struct NfcEvent {
  NfcEventType type;
  // Scan 이벤트일 때만 UID 사용
  char uid[32];
};

class Nfc {
public:
  explicit Nfc(NfcPinConfig pinConfig);

  // --------------------------------------------------------------------------
  // begin(actions)
  // - RC522 초기화
  // - 이벤트 큐 생성
  // - read/callback 태스크 시작
  // --------------------------------------------------------------------------
  void begin(NfcActions actions);

  // 모든 RTOS 리소스(태스크/큐/뮤텍스) 정리
  void stopTask();

  // stopTask() 후 begin() 재호출
  void restart(NfcActions actions);

  // 폴링 주기(ms). 너무 작으면 CPU 부하, 너무 크면 반응 속도 저하.
  void setPollInterval(uint32_t ms);

  // 최근 판단 기준 카드 존재 상태
  bool isCardDetected();

  // RC522 통신 강제 재초기화
  void restartCommunication();

private:
  static void readTaskThunk(void *pv);
  static void callbackTaskThunk(void *pv);

  void readTaskLoop();
  void callbackTaskLoop();
  void read();

private:
  MFRC522 mfrc522;

  // 마지막으로 감지된 UID 문자열
  char preNfcCard[32] = {0};

  // 카드 존재 플래그
  bool isCardPresent = false;

  // 미검출 누적 카운터(디바운싱 유사 용도)
  uint8_t igoneTime = 0;

  // 폴링 주기
  uint32_t pollIntervalMs = 40;

  NfcActions nfcActions;

  TaskHandle_t readTaskHandle = nullptr;
  TaskHandle_t callbackTaskHandle = nullptr;

  QueueHandle_t eventQueue = nullptr;

  // SPI 공유 보호
  SemaphoreHandle_t spiMutex = nullptr;
};

#endif
