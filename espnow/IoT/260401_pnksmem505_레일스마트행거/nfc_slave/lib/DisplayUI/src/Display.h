#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <TaskRunner.h>

class Display {
public:
  struct Config {
    uint8_t rotation;
    int8_t powerPin;
    bool powerHigh;
    uint16_t powerDelayMs;
    TaskRunner *task;
    uint16_t loadingIntervalMs;
  };

  struct ShowText {
    const char *title;
    const char *value;
  };

  Display(const Config &config);

  void begin();
  void showText(const ShowText &config);
  void startLoading(const ShowText &config);
  void stopLoading();

private:
  struct ShowLoading {
    const char *title;
    const char *value;
    uint8_t frame;
  };

  static void taskLoopThunk(void *context);
  void taskLoop();
  void showLoading(const ShowLoading &config);

  Config _config;
  TFT_eSPI _tft;
  TFT_eSprite _sprite;
  bool _spriteReady;
  TaskRunner *_task;
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
  portMUX_TYPE _drawMux = portMUX_INITIALIZER_UNLOCKED;
  bool _loading;
  uint8_t _frame;
  uint32_t _lastFrameMs;
  uint32_t _loadingToken;
  String _loadingTitle;
  String _loadingValue;
  uint16_t _loadingIntervalMs;
};
