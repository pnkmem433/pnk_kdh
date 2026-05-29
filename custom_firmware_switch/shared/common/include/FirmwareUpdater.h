#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include "AppConfig.h"
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
                             std::function<void(FirmwareUpdateResult)> resultCallback);
  void setAutoReset(bool enable);
  void reset();

private:
#if defined(ESP32)
  bool migrateViaSafeboot(std::function<void(float)> progressCallback);
  String buildMigrationSafebootUrl() const;
  bool writeFactoryPartitionFromUrl(const String& binaryUrl,
                                    std::function<void(float)> progressCallback);
#endif
  String firmwareServerUrl;
  int firmwareProjectId;
  int currentFirmwareVersionNumber;
  String currentChipType;
  String currentFirmwareFamily;
  bool autoReset;
};

#endif
