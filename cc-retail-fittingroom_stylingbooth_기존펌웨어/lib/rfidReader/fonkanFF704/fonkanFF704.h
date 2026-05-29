#ifndef FONKAN_FF704_H
#define FONKAN_FF704_H

#include "../rfidReader.h"
#include <HardwareSerial.h>
#include <freertos/semphr.h>

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
  bool getPowerLevelDbm(int &outDbm) override;
  bool setPowerLevelDbm(int dbm) override;

private:
  HardwareSerial &serial;
  uint32_t baudRate;
  std::function<void(String)> callback;
  int8_t rxPin;
  int8_t txPin;
  TaskHandle_t taskHandle1;
  TaskHandle_t taskHandle2;
  SemaphoreHandle_t tagsMutex;
  SemaphoreHandle_t serialMutex;
  volatile bool readingEnabled;
  byte readCommand[3] = {0x0A, 0x55, 0x0D};
  std::map<String, int> tags;

  bool sendAsciiCommandAndReadReply(const String &command, String &reply,
                                    uint32_t timeoutMs = 400);
};

#include "fonkanFF704.cpp"

#endif
