/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - FF701 리더 클래스 선언 및 명령 상수 정의.
*/
#ifndef FONKAN_FF701_H
#define FONKAN_FF701_H

#include "../rfidReader.h"
#include <HardwareSerial.h>

/*
  FonkanFF701RfidReader
  - FF-701 프로토콜(start/stop 명령) 기반 리더 구현.
*/
class FonkanFF701RfidReader : public RfidReader {
public:
  FonkanFF701RfidReader(HardwareSerial &serial, int8_t rxPin, int8_t txPin,
                        uint32_t baudrate);
  void begin() override;

  void startRead() override;
  void stopRead() override;

  std::map<String, int> getTags() override;
  void clearTags() override;

  void onRead(std::function<void(String)> callback) override;

  String readerName() override;
  bool getPowerLevelDbm(int &outDbm) override;
  bool setPowerLevelDbm(int dbm) override;

private:
  HardwareSerial &serial;
  uint32_t baudRate;
  std::function<void(String)> callback;

  int8_t rxPin;
  int8_t txPin;

  TaskHandle_t taskHandle;

  byte startCommand[5] = {0x40, 0x03, 0x0F, 0x01, 0xAD};
  byte stopCommand[5] = {0x40, 0x03, 0x0F, 0x00, 0xAE};

  std::map<String, int> tags;
};

#include "fonkanFF701.cpp"

#endif


