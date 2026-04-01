/*
  [파일 설명] lib/nfc/nfc.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "nfc.h"

Nfc::Nfc(NfcPinConfig pinConfig) : mfrc522(pinConfig.ssPin, pinConfig.rstPin) {}

// begin(): RC522/큐/태스크를 초기화하고 감지를 시작한다.
void Nfc::begin(NfcActions actions) {
  // 사용자 콜백 저장
  nfcActions = std::move(actions);

  // RC522 초기화
  // begin()이 재호출될 수 있으므로, 불안정 시 restart() 경로로 재초기화 가능
  SPI.begin();
  delay(50);
  mfrc522.PCD_Init();

  // SPI 보호용 뮤텍스 생성
  if (spiMutex == nullptr) {
    spiMutex = xSemaphoreCreateMutex();
  }

  // NFC 이벤트 큐 생성
  // 길이 5는 순간 이벤트 버스트를 흡수하기 위한 최소 버퍼
  eventQueue = xQueueCreate(5, sizeof(NfcEvent));

  // 태스크 시작(중복 생성 방지)
  // readTask: 감지 전용 / callbackTask: 콜백 실행 전용
  if (readTaskHandle == nullptr) {
    xTaskCreate(readTaskThunk, "nfc_read_task", 4096, this, 3, &readTaskHandle);
  }

  if (callbackTaskHandle == nullptr) {
    xTaskCreate(callbackTaskThunk, "nfc_cb_task", 4096, this, 4,
                &callbackTaskHandle);
  }
}

// stopTask(): read/callback 태스크와 큐/뮤텍스를 모두 정리한다.
void Nfc::stopTask() {
  if (readTaskHandle) {
    vTaskDelete(readTaskHandle);
    readTaskHandle = nullptr;
  }

  if (callbackTaskHandle) {
    vTaskDelete(callbackTaskHandle);
    callbackTaskHandle = nullptr;
  }

  if (eventQueue) {
    vQueueDelete(eventQueue);
    eventQueue = nullptr;
  }

  if (spiMutex) {
    vSemaphoreDelete(spiMutex);
    spiMutex = nullptr;
  }
}

// restart(): 통신 이상 복구용 재시작 헬퍼
void Nfc::restart(NfcActions actions) {
  stopTask();
  begin(std::move(actions));
}

void Nfc::setPollInterval(uint32_t ms) { pollIntervalMs = ms; }

void Nfc::readTaskThunk(void *pv) { static_cast<Nfc *>(pv)->readTaskLoop(); }

void Nfc::callbackTaskThunk(void *pv) {
  static_cast<Nfc *>(pv)->callbackTaskLoop();
}

// readTaskLoop(): pollInterval 간격으로 NFC 읽기 반복
void Nfc::readTaskLoop() {
  for (;;) {
    // 1회 폴링 후 잠시 대기
    read();
    vTaskDelay(pdMS_TO_TICKS(pollIntervalMs));
  }
}

// callbackTaskLoop(): 큐 이벤트를 사용자 콜백으로 전달
void Nfc::callbackTaskLoop() {
  NfcEvent event;

  for (;;) {
    // 큐에 이벤트가 도착할 때까지 대기
    if (xQueueReceive(eventQueue, &event, portMAX_DELAY) == pdTRUE) {
      switch (event.type) {
      case NfcEventType::Scan:
        if (nfcActions.onScan) {
          nfcActions.onScan(event.uid);
        }
        break;

      case NfcEventType::Remove:
        if (nfcActions.onRemove) {
          nfcActions.onRemove();
        }
        break;

      case NfcEventType::preRemove:
        if (nfcActions.onPrepareForRemoval) {
          nfcActions.onPrepareForRemoval();
        }
        break;
      }
    }
  }
}

/*
  read()
  - 카드 UID를 읽고 상태 전이를 판단한다.
  - Scan/preRemove/Remove 이벤트를 큐에 넣는다.
*/
void Nfc::read() {
  // SPI 공유 구간 보호
  if (xSemaphoreTake(spiMutex, portMAX_DELAY) != pdTRUE) {
    return;
  }

  char detectedUid[32] = {0};
  bool hasUid = false;

  // 새 카드 존재 여부 + UID 읽기
  // PICC_IsNewCardPresent() 호출 뒤 ReadCardSerial()로 실제 UID를 읽는다.
  mfrc522.PICC_IsNewCardPresent();
  bool isCardRead = mfrc522.PICC_ReadCardSerial();

  if (isCardRead) {
    char *ptr = detectedUid;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (ptr - detectedUid < (int)sizeof(detectedUid) - 3) {
        ptr += sprintf(ptr, "%02X", mfrc522.uid.uidByte[i]);
      }
    }
    hasUid = (ptr > detectedUid);
  }

  if (hasUid) {
    // 감지 성공 시 미검출 카운터 초기화
    igoneTime = 0;

    // 이전 UID와 다를 때만 새 Scan 이벤트 발행(중복 억제)
    // 동일 카드가 계속 붙어있는 동안 이벤트 폭주 방지
    if (strncmp(preNfcCard, detectedUid, sizeof(preNfcCard)) != 0) {
      strncpy(preNfcCard, detectedUid, sizeof(preNfcCard));
      isCardPresent = true;

      if (eventQueue) {
        NfcEvent event;
        event.type = NfcEventType::Scan;
        strncpy(event.uid, detectedUid, sizeof(event.uid));
        xQueueSend(eventQueue, &event, 0);
      }
    }

  } else if (igoneTime < 2) {
    // 짧은 미검출 구간은 즉시 Remove로 판단하지 않는다.
    // 카드 흔들림/간섭/각도 문제를 흡수하기 위한 완충 로직
    // 2회 누적 전에 preRemove를 1회 보내 복구 훅 실행 기회 제공
    if (igoneTime == 1) {
      NfcEvent event;
      event.type = NfcEventType::preRemove;
      xQueueSend(eventQueue, &event, 0);
    }
    igoneTime++;

  } else if (preNfcCard[0] != '\0') {
    // 연속 미검출이 충분히 누적되면 Remove 확정
    // 카드 없음 상태로 전이하고 Remove 이벤트를 큐에 전달
    preNfcCard[0] = '\0';
    isCardPresent = false;

    if (eventQueue) {
      NfcEvent event;
      event.type = NfcEventType::Remove;
      event.uid[0] = '\0';
      xQueueSend(eventQueue, &event, 0);
    }
  }

  xSemaphoreGive(spiMutex);
}

bool Nfc::isCardDetected() { return isCardPresent; }

void Nfc::restartCommunication() {
  // RC522가 불안정할 때 SPI + RC522를 재초기화한다.
  if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE) {
    SPI.begin();
    mfrc522.PCD_Init();
    xSemaphoreGive(spiMutex);
  }
}

