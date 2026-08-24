/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - FF701 start/stop 명령과 수신 파싱 태스크 구현.
*/
/*
  모듈 목적:
  - Fonkan FF-701 리더를 UART로 제어(start/stop)하고 태그 문자열을 누적 카운트한다.

  입력:
  - UART 수신 라인("Tag:...")

  출력:
  - tags 맵 누적, onRead 콜백 호출

  동시성 포인트:
  - startRead()에서 전용 태스크 1개를 생성해 수신 파싱을 수행한다.

  실패 시 동작:
  - 포맷이 맞지 않는 라인은 무시한다.
*/
#include "fonkanFF701.h"

/*
  FonkanFF701RfidReader(serial, rxPin, txPin, baudrate)
  - FF701 UART 파라미터를 저장한다.
*/
FonkanFF701RfidReader::FonkanFF701RfidReader(HardwareSerial &serial,
                                             int8_t rxPin, int8_t txPin,
                                             uint32_t baudrate)
    : serial(serial), baudRate(baudrate), taskHandle(nullptr), txPin(txPin),
      rxPin(rxPin) {}

/*
  begin()
  - 지정한 UART 설정으로 시리얼 포트를 연다.
*/
void FonkanFF701RfidReader::begin() {
  serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
}

/*
  startRead()
  - FF701 시작 명령 전송 후 수신 파싱 태스크를 시작한다.
*/
void FonkanFF701RfidReader::startRead() {
  serial.write(startCommand, 5);
  if (taskHandle == nullptr) {
    xTaskCreate(
        [](void *pv) {
          FonkanFF701RfidReader *reader =
              static_cast<FonkanFF701RfidReader *>(pv);

          while (true) {
            if (reader->serial.available()) {
              String data = reader->serial.readStringUntil('\n');
              int index = data.indexOf("Tag:");
              if (index != -1) {
                String tag = data.substring(index + 4);
                tag.trim();
                if (reader->callback != nullptr) {
                  reader->callback(tag);
                }
                // 동일 태그 반복 출현 횟수 누적(후속 신뢰도 필터에 사용)
                reader->tags[tag]++;
              }
            }
            vTaskDelay(2 / portTICK_PERIOD_MS);
          }
        },
        "ff701_task", 4096, this, 1, &taskHandle);
  }
}

/*
  stopRead()
  - FF701 중지 명령 전송 및 파싱 태스크 정지.
*/
void FonkanFF701RfidReader::stopRead() {
  serial.write(stopCommand, 5);
  if (taskHandle != nullptr) {
    vTaskDelete(taskHandle);
    taskHandle = nullptr;
  }
}

// 누적 태그 카운트 초기화
void FonkanFF701RfidReader::clearTags() { tags.clear(); }

// 현재 누적 태그 카운트 맵 반환
std::map<String, int> FonkanFF701RfidReader::getTags() { return tags; }

// 태그 수신 콜백 등록
void FonkanFF701RfidReader::onRead(std::function<void(String)> cb) {
  callback = cb;
}

// 리더 모델명 반환
String FonkanFF701RfidReader::readerName() { return "Fonkan FF-701"; }


