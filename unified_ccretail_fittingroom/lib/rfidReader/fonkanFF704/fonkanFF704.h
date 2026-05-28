/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - FF704 리더 클래스 선언과 태스크 핸들 관리 정의.
*/
#ifndef FONKAN_FF704_H
#define FONKAN_FF704_H

#include "../rfidReader.h"
#include <HardwareSerial.h>
#include "freertos/semphr.h"

/*
  FonkanFF704RfidReader
  - FF-704 리더 구현.
  - 태스크 2개 사용:
    - send task: 100ms 주기로 read command 송신
    - read task: 수신 태그 파싱/카운팅
*/
class FonkanFF704RfidReader : public RfidReader {
public:
  FonkanFF704RfidReader(HardwareSerial &serial, int8_t rxPin, int8_t txPin,
                        uint32_t baudrate);
  void begin() override;

  void startRead() override;
  void stopRead() override;

  std::map<String, int> getTags() override;
  void clearTags() override;

  void onRead(std::function<void(String)> callback) override;

  String readerName() override;

private:
  HardwareSerial &serial;
  uint32_t baudRate;
  std::function<void(String)> callback;

  int8_t rxPin;
  int8_t txPin;

  TaskHandle_t taskHandle1;
  TaskHandle_t taskHandle2;
  SemaphoreHandle_t tagsMutex;
  volatile bool readingEnabled;

  byte readCommand[3] = {0x0A, 0x55, 0x0D};

  std::map<String, int> tags;
};

#include "fonkanFF704.cpp"

#endif
