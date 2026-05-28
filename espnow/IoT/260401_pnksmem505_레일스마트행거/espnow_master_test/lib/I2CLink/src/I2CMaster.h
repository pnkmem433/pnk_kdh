#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <array>
#include <initializer_list>
#include <TaskRunner.h>

class I2CMaster {
public:
  static const uint8_t VALUE_MAX = 64;
  static const uint8_t CMD_ECHO = 0xA1;
  static const size_t MAX_SLAVES = 8;
  static const uint8_t RX_MAX = 120; // 한 번에 받을 최대 바이트 수

  struct Config {
    std::initializer_list<uint8_t> slaves; // 슬레이브 주소 목록
    int sda;                                // SDA 핀(GPIO)
    int scl;                                // SCL 핀(GPIO)
    uint32_t frequency;                     // I2C 주파수(Hz)
    TaskRunner *task;                       // 스캔을 돌릴 TaskRunner
  };

  struct RequestValueString {
    uint8_t address; // 요청 대상 슬레이브 주소
  };

  struct SendEchoString {
    uint8_t address; // 에코 대상 슬레이브 주소
    String value;    // 에코로 보낼 문자열
  };

  struct LinkData {
    uint8_t address; // 데이터를 보낸 슬레이브 주소
    String value;    // 슬레이브에서 온 값
  };

  struct SlaveScan {
    String (*onDataReceive)(const LinkData &data); // 값 수신 시 호출, 반환 문자열을 에코로 사용
  };

  struct Begin {
    String (*onDataReceive)(const LinkData &data); // 값 수신 시 호출될 처리 함수
    uint32_t intervalMs;                            // 스캔 주기(ms)
    uint32_t startDelayMs;                          // 시작 지연(ms)
  };

  I2CMaster(const Config &config);

  void begin();
  void begin(const Begin &begin);
  size_t slaveCount() const;

  struct SlaveRange {
    const uint8_t *data;
    size_t count;

    const uint8_t *begin() const { return data; }
    const uint8_t *end() const { return data + count; }
  };
  SlaveRange slaves() const;

  String requestValueString(const RequestValueString &request);
  void sendEchoString(const SendEchoString &echo);
  void scanSlaves(const SlaveScan &scan);

private:
  bool requestValueRaw(uint8_t address, uint8_t *outValue, uint8_t &outLen);
  void sendEchoRaw(uint8_t address, const uint8_t *value, uint8_t len);
  static uint16_t crc16ccitt(const uint8_t *data, uint8_t len);
  static void taskLoopThunk(void *context);
  void taskLoop();

  Config _config;
  std::array<uint8_t, MAX_SLAVES> _slaves;
  size_t _slaveCount;

  TaskRunner *_taskRunner;
  String (*_onDataReceive)(const LinkData &data);
  uint32_t _intervalMs;
  uint32_t _startDelayMs;
};
