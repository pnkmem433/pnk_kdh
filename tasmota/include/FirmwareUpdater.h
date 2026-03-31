#pragma once

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <functional>

enum class FirmwareUpdateResult {
  Success,
  NoUpdate,
  Failed
};

class FirmwareUpdater {
public:
  explicit FirmwareUpdater(const char* serverUrl);

  void performFirmwareUpdate(std::function<void(float)> progressCallback,
                             std::function<void(FirmwareUpdateResult)> resultCallback);

private:
  String updateUrl() const;

  const char* serverUrl_;
};
