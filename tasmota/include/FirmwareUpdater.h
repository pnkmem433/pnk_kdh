#pragma once

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <functional>

enum class FirmwareUpdateResult {
  Success,
  NoUpdate,
  Failed,
  ProjectNotFound,
  VersionNotFound,
  FileNotFound
};

class FirmwareUpdater {
public:
  FirmwareUpdater(const char* serverUrl, const char* projectId, int versionCode);

  void performFirmwareUpdate(std::function<void(float)> progressCallback,
                             std::function<void(FirmwareUpdateResult)> resultCallback);
  void setAutoReset(bool enable);
  void reset();

private:
  String requestUrl() const;

  const char* serverUrl_;
  const char* projectId_;
  int versionCode_;
  bool autoReset_ = true;
  uint8_t updateBuffer_[128];
};
