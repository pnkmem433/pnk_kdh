#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <functional>
#include <TaskRunner.h>

struct NfcPins
{
  uint8_t SCK;
  uint8_t MISO;
  uint8_t MOSI;
  uint8_t SS;
  uint8_t RST;
  int8_t IRQ; // optional, active LOW
};

struct NfcConfig
{
  NfcPins pin;
  TaskRunner *task; // NFC 스캔을 돌릴 TaskRunner
  struct NfcSettings
  {
    int miss; // ms, 0 = immediate, <0 = disabled
    bool irqMode;
    uint16_t pollIntervalMs;
    uint8_t reinitMisses;
  } settings;
};

class NfcReader
{
public:
  using ReadCallback = std::function<void(const String &)>;
  using RemoveCallback = std::function<void()>;
  using ErrorCallback = std::function<void(const String &)>;
  struct Callbacks
  {
    ReadCallback onRead;
    RemoveCallback onRemove;
    ErrorCallback onError;
  };

  NfcReader(const NfcConfig &config);

  void begin();
  void begin(const Callbacks &cbs);
  void reset();

  void onRead(ReadCallback cb);
  void onRemove(RemoveCallback cb);
  void onError(ErrorCallback cb);
  void setCallbacks(const Callbacks &cbs);
  uint8_t version() const;
  bool hasError() const;
  const String &lastError() const;

private:
  NfcConfig::NfcSettings _settings;
  uint16_t _removeTimeMs = 0;
  bool _irqMode = false;
  static void taskThunk(void *context);
  void taskTick();
  bool checkHardware();
  String uidToHex(const uint8_t *uid, uint8_t uidLength);
  void reinitIfNeeded();

  uint8_t _sck;
  uint8_t _miso;
  uint8_t _mosi;
  uint8_t _ss;
  uint8_t _rst;
  int8_t _irq = -1;

  MFRC522 _rfid;
  ReadCallback _onRead;
  RemoveCallback _onRemove;
  ErrorCallback _onError;

  String _lastUid;
  bool _cardPresent = false;
  uint8_t _failCount = 0;
  uint8_t _missCount = 0;
  uint32_t _lastPollMs = 0;
  uint32_t _lastSeenMs = 0;
  TaskRunner *_task;
  bool _ok = false;
  uint32_t _lastCheckMs = 0;
  uint8_t _version = 0;
  uint32_t _lastLogMs = 0;
  uint8_t _lastReqStatus = 0xFF;
  String _lastError;
};
