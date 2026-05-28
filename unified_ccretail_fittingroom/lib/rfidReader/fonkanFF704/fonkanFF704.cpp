/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - FF704 송신/수신 분리 태스크와 태그 누적 로직 구현.
*/
/*
  모듈 목적:
  - Fonkan FF-704 리더를 UART로 주기 폴링하고 수신 태그를 누적 카운트한다.

  입력:
  - readCommand(0x0A,0x55,0x0D) 송신 응답 프레임

  출력:
  - tags 맵 누적, onRead 콜백 호출

  동시성 포인트:
  - taskHandle1: 100ms 주기 read command 송신
  - taskHandle2: 수신 파싱
  - 태스크 분리 이유: 송신 주기 안정성과 수신 처리 지연의 상호 영향 최소화
*/
#include "fonkanFF704.h"
#include <ctype.h>

namespace {
constexpr uint64_t RFID_POLL_INTERVAL_MS = 50;
constexpr uint32_t RFID_SERIAL_TIMEOUT_MS = 30;

String normalizeTag(String raw) {
  raw.trim();

  String cleaned = "";

  for (size_t i = 0; i < raw.length(); ++i) {
    unsigned char c = static_cast<unsigned char>(raw[i]);

    if (!isprint(c) || isspace(c)) {
      continue;
    }

    cleaned += static_cast<char>(toupper(c));
  }

  if (cleaned.length() != 33) {
    return "";
  }

  if (cleaned[0] != 'U') {
    return "";
  }

  for (size_t i = 1; i < cleaned.length(); ++i) {
    char c = cleaned[i];
    bool isHex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
    if (!isHex) {
      return "";
    }
  }

  return cleaned;
}
} // namespace

/*
  FonkanFF704RfidReader(serial, rxPin, txPin, baudrate)
  - FF704 UART 파라미터 및 태스크 핸들 초기화.
*/
FonkanFF704RfidReader::FonkanFF704RfidReader(HardwareSerial &serial,
                                             int8_t rxPin, int8_t txPin,
                                             uint32_t baudrate)
    : serial(serial), baudRate(baudrate), taskHandle1(nullptr),
      taskHandle2(nullptr), tagsMutex(nullptr), readingEnabled(false),
      txPin(txPin), rxPin(rxPin) {}

/*
  begin()
  - FF704 통신용 UART를 연다.
*/
void FonkanFF704RfidReader::begin() {
  serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
  serial.setTimeout(RFID_SERIAL_TIMEOUT_MS);
  if (tagsMutex == nullptr) {
    tagsMutex = xSemaphoreCreateMutex();
  }
}

/*
  startRead()
  - 송신 태스크(taskHandle1): 100ms 주기로 readCommand 전송
  - 수신 태스크(taskHandle2): 수신 프레임 파싱/태그 카운트 누적
*/
void FonkanFF704RfidReader::startRead() {
  readingEnabled = true;
  while (serial.available()) {
    serial.read();
  }

  if (taskHandle1 == nullptr) {
    xTaskCreate(
        [](void *pv) {
          FonkanFF704RfidReader *reader =
              static_cast<FonkanFF704RfidReader *>(pv);

          uint64_t pretime = esp_timer_get_time() / 1000;

          while (true) {
            if (!reader->readingEnabled) {
              pretime = esp_timer_get_time() / 1000;
              vTaskDelay(pdMS_TO_TICKS(10));
              continue;
            }

            uint64_t nowtime = esp_timer_get_time() / 1000;
            // 50ms polling command: 감지 빈도를 높여 근접 정지 태그 재검출 강화
            if (nowtime - pretime >= RFID_POLL_INTERVAL_MS) {
              pretime += RFID_POLL_INTERVAL_MS;
              reader->serial.write(reader->readCommand, 3);
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
        },
        "ff704_task_send_command", 4096, this, 2, &taskHandle1);
  }

  if (taskHandle2 == nullptr) {
    xTaskCreate(
        [](void *pv) {
          FonkanFF704RfidReader *reader =
              static_cast<FonkanFF704RfidReader *>(pv);

          while (true) {
            if (!reader->readingEnabled) {
              while (reader->serial.available()) {
                reader->serial.read();
              }
              vTaskDelay(pdMS_TO_TICKS(10));
              continue;
            }

            if (reader->serial.available()) {
              String raw = reader->serial.readStringUntil(0x0A);
              String tag = normalizeTag(raw);

              if (tag.length() > 0) {
                if (reader->callback != nullptr) {
                  reader->callback(tag);
                }
                if (reader->tagsMutex != nullptr &&
                    xSemaphoreTake(reader->tagsMutex, portMAX_DELAY) == pdTRUE) {
                  reader->tags[tag]++;
                  xSemaphoreGive(reader->tagsMutex);
                }
              }
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
        },
        "ff704_task_read_command", 4096, this, 1, &taskHandle2);
  }
}

/*
  stopRead()
  - 송신/수신 태스크는 유지하고, 읽기 활성화만 중단한다.
*/
void FonkanFF704RfidReader::stopRead() {
  readingEnabled = false;
  while (serial.available()) {
    serial.read();
  }
}

// 누적 태그 카운트 초기화
void FonkanFF704RfidReader::clearTags() {
  if (tagsMutex != nullptr &&
      xSemaphoreTake(tagsMutex, portMAX_DELAY) == pdTRUE) {
    tags.clear();
    xSemaphoreGive(tagsMutex);
  }
}

// 누적 태그 카운트 맵 반환
std::map<String, int> FonkanFF704RfidReader::getTags() {
  std::map<String, int> snapshot;
  if (tagsMutex != nullptr &&
      xSemaphoreTake(tagsMutex, portMAX_DELAY) == pdTRUE) {
    snapshot = tags;
    xSemaphoreGive(tagsMutex);
  }
  return snapshot;
}

// 태그 수신 콜백 등록
void FonkanFF704RfidReader::onRead(std::function<void(String)> cb) {
  callback = cb;
}

// 리더 모델명 반환
String FonkanFF704RfidReader::readerName() { return "Fonkan FF-704"; }
