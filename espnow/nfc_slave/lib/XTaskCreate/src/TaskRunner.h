#pragma once

#include <Arduino.h>

class TaskRunner {
public:
  using LoopFunction = void (*)();
  using LoopFunctionWithContext = void (*)(void *);

  struct Config {
    const char *name;     // 태스크 이름
    uint32_t stackSize;   // 스택 크기(바이트)
    UBaseType_t priority; // 태스크 우선순위
    int8_t core;          // -1 = 코어 고정 안 함
  };

  struct Begin {
    LoopFunction loop;                // 루프 함수(컨텍스트 없음)
    LoopFunctionWithContext loopWithContext; // 컨텍스트 받는 루프 함수
    void *context;                    // 루프 함수로 전달할 컨텍스트
  };

  TaskRunner(const Config &config);

  void begin();
  void begin(const Begin &begin);

private:
  static void entry(void *param);

  Config _config;
  TaskHandle_t _handle;
  LoopFunction _loop;
  LoopFunctionWithContext _loopWithContext;
  void *_context;
  bool _useContext;
};
