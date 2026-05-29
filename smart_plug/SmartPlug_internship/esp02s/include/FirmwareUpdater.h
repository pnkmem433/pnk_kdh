#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>
#include <functional>

enum class FirmwareUpdateResult {
  SUCCESS,
  PROJECT_NOT_FOUND,
  VERSION_NOT_FOUND,
  NO_UPDATE_NEEDED,
  FILE_NOT_FOUND,
  FAILED
};

class FirmwareUpdater {
public:
  FirmwareUpdater(const String& url,
                  int projectId,
                  int firmwareVersion,
                  const String& chipType,
                  const String& firmwareFamily);

  void performFirmwareUpdate(std::function<void(float)> progressCallback,
                             std::function<void(FirmwareUpdateResult)> resultCallback,
                             std::function<void()> tickCallback = nullptr);
  void setDebugLogger(std::function<void(const String&)> logger);
  void setBootSourceHint(const String& hint);
  void setAutoReset(bool enable);
  void reset();

private:
  void debugLog(const String& message);
  void debugUpdateError(const char* context);
  String firmwareServerUrl;
  int firmwareProjectId;
  int currentFirmwareVersionNumber;
  String currentChipType;
  String currentFirmwareFamily;
  String bootSourceHint;
  bool autoReset;
  std::function<void(const String&)> debugLogger;
};

#endif

