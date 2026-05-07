#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <TaskRunner.h>

class I2CSlave {
public:
  static const uint8_t VALUE_MAX = 64;
  static const uint8_t CMD_ECHO = 0xA1;
  static const uint8_t TX_MAX = 120; // 한 번에 보낼 최대 바이트 수(버퍼 한계)
  static const uint8_t QUEUE_MAX = 8; // 전송 큐 최대 개수(하드 상한)

  struct Config {
    uint8_t address;    // 이 슬레이브의 I2C 주소
    int sda;            // SDA 핀(GPIO 번호)
    int scl;            // SCL 핀(GPIO 번호)
    uint32_t frequency; // I2C 버스 주파수(Hz)
    TaskRunner *task;   // 에코 수신을 돌릴 TaskRunner
    uint8_t queueMax;   // 전송 큐 최대 개수(0이면 기본값)
  };

  struct Send {
    String value; // 보낼 값(문자열)
  };

  struct Begin {
    void (*onReceive)(const String &value); // 에코 수신 시 호출될 처리 함수
    uint32_t intervalMs;                     // 스캔 주기(ms)
    uint32_t startDelayMs;                   // 시작 지연(ms)
  };

  I2CSlave(const Config &config);

  void begin();
  void begin(const Begin &begin);
  void send(const Send &send);
  bool takeEchoString(String &out);
  void scanEcho(const Begin &scan);

private:
  struct QueueItem {
    uint8_t len;
    uint8_t data[VALUE_MAX];
  };

  static void onRequestThunk();
  static void onReceiveThunk(int count);
  void onRequest();
  void onReceive(int count);
  static void taskLoopThunk(void *context);
  void taskLoop();
  static uint16_t crc16ccitt(const uint8_t *data, uint8_t len);

  static I2CSlave *_instance;
  Config _config;

  QueueItem _queue[QUEUE_MAX];
  uint8_t _qHead;
  uint8_t _qTail;
  uint8_t _qCount;
  uint8_t _qMax;

  volatile bool _echoUpdated;
  volatile uint8_t _echoLen;
  volatile uint8_t _echoData[VALUE_MAX];
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

  TaskRunner *_taskRunner;
  void (*_onReceive)(const String &value);
  uint32_t _intervalMs;
  uint32_t _startDelayMs;
};
