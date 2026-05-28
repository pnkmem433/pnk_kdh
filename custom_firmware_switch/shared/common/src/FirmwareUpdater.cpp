#include "FirmwareUpdater.h"

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <Updater.h>
#else
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#endif

FirmwareUpdater::FirmwareUpdater(const String& url, int projectId,
                                 int firmwareVersion,
                                 const String& chipType,
                                 const String& firmwareFamily)
    : firmwareServerUrl(url),
      firmwareProjectId(projectId),
      currentFirmwareVersionNumber(firmwareVersion),
      currentChipType(chipType),
      currentFirmwareFamily(firmwareFamily),
      autoReset(true) {}

void FirmwareUpdater::performFirmwareUpdate(
    std::function<void(float)> progressCallback,
    std::function<void(FirmwareUpdateResult)> resultCallback) {
  FirmwareUpdateResult result = FirmwareUpdateResult::FAILED;

  WiFiClient client;
  HTTPClient httpClient;
  if (!httpClient.begin(client, firmwareServerUrl + "/firmwareDownload")) {
    if (resultCallback) {
      resultCallback(result);
    }
    return;
  }

  httpClient.addHeader("Content-Type", "application/json");
  const char* headerKeys[] = {"X-Target-Firmware-Family"};
  httpClient.collectHeaders(headerKeys, 1);
  const String payload =
      "{\"projectId\": " + String(firmwareProjectId) +
      ", \"chipType\": \"" + currentChipType +
      "\", \"currentFirmwareFamily\": \"" + currentFirmwareFamily +
      "\", \"currentVersion\": " + String(currentFirmwareVersionNumber) + "}";
  const int responseCode = httpClient.POST(payload);

  if (responseCode == 201) {
#if defined(ESP32)
    const String targetFamily = httpClient.header("X-Target-Firmware-Family");
    if (currentFirmwareFamily.equalsIgnoreCase("custom") &&
        targetFamily.equalsIgnoreCase("tasmota")) {
      httpClient.end();
      if (migrateViaSafeboot(progressCallback)) {
        result = FirmwareUpdateResult::SUCCESS;
      }
      if (resultCallback) {
        resultCallback(result);
      }
      if (autoReset && result == FirmwareUpdateResult::SUCCESS) {
        reset();
      }
      return;
    }
#endif
    const int totalSize = httpClient.getSize();
    WiFiClient* stream = httpClient.getStreamPtr();

    if (totalSize > 0 && Update.begin((size_t)totalSize)) {
      size_t written = 0;
      float lastProgress = 0.0f;
      uint8_t buffer[256];

      while (httpClient.connected() && (written < (size_t)totalSize)) {
        const size_t available = stream->available();
        if (!available) {
          delay(1);
          continue;
        }

        const size_t toRead = min(sizeof(buffer), available);
        const size_t readCount = stream->readBytes(buffer, toRead);
        if (readCount == 0) {
          continue;
        }

        written += Update.write(buffer, readCount);
        if (progressCallback && totalSize > 0) {
          const float progress =
              static_cast<float>(written) / static_cast<float>(totalSize);
          if (progress - lastProgress >= 0.03f) {
            lastProgress = progress;
            progressCallback(progress);
          }
        }
      }

      if (Update.end(true)) {
        result = FirmwareUpdateResult::SUCCESS;
      }
    }
  } else {
    const String responseBody = httpClient.getString();
    if (responseBody.indexOf("프로젝트를 찾을 수 없습니다.") != -1) {
      result = FirmwareUpdateResult::PROJECT_NOT_FOUND;
    } else if (responseBody.indexOf("프로젝트 버전을 찾을 수 없습니다.") != -1 ||
               responseBody.indexOf("해당 칩 종류에 맞는 버전을 찾을 수 없습니다.") != -1 ||
               responseBody.indexOf("해당 칩 종류에 맞는 업그레이드 후보를 찾을 수 없습니다.") != -1) {
      result = FirmwareUpdateResult::VERSION_NOT_FOUND;
    } else if (responseBody.indexOf("현재 버전보다 높은 같은 계열의 업데이트가 없습니다.") != -1) {
      result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
    } else if (responseBody.indexOf("bin 파일을 찾을 수 없습니다.") != -1) {
      result = FirmwareUpdateResult::FILE_NOT_FOUND;
    }
  }

  httpClient.end();

  if (resultCallback) {
    resultCallback(result);
  }

  if (autoReset && result == FirmwareUpdateResult::SUCCESS) {
    reset();
  }
}

void FirmwareUpdater::setAutoReset(bool enable) { autoReset = enable; }

void FirmwareUpdater::reset() { ESP.restart(); }

#if defined(ESP32)
bool FirmwareUpdater::migrateViaSafeboot(
    std::function<void(float)> progressCallback) {
  return writeFactoryPartitionFromUrl(buildMigrationSafebootUrl(),
                                      progressCallback);
}

String FirmwareUpdater::buildMigrationSafebootUrl() const {
  return firmwareServerUrl + "/firmwareDownload/migration-safeboot?chipType=" +
         currentChipType;
}

bool FirmwareUpdater::writeFactoryPartitionFromUrl(
    const String& binaryUrl, std::function<void(float)> progressCallback) {
  WiFiClient client;
  HTTPClient httpClient;
  if (!httpClient.begin(client, binaryUrl)) {
    return false;
  }

  const int responseCode = httpClient.GET();
  if (responseCode != 200) {
    httpClient.end();
    return false;
  }

  const esp_partition_t* factoryPartition = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "safeboot");
  if (factoryPartition == nullptr) {
    factoryPartition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, nullptr);
  }
  if (factoryPartition == nullptr) {
    httpClient.end();
    return false;
  }

  const int totalSize = httpClient.getSize();
  if (totalSize <= 0 || static_cast<size_t>(totalSize) > factoryPartition->size) {
    httpClient.end();
    return false;
  }

  const size_t eraseSize =
      ((static_cast<size_t>(totalSize) + 0xFFF) / 0x1000) * 0x1000;
  if (esp_partition_erase_range(factoryPartition, 0, eraseSize) != ESP_OK) {
    httpClient.end();
    return false;
  }

  WiFiClient* stream = httpClient.getStreamPtr();
  size_t written = 0;
  uint8_t buffer[1024];

  while (httpClient.connected() && written < static_cast<size_t>(totalSize)) {
    const size_t available = stream->available();
    if (!available) {
      delay(1);
      continue;
    }

    const size_t toRead = min(sizeof(buffer), available);
    const size_t readCount = stream->readBytes(buffer, toRead);
    if (readCount == 0) {
      continue;
    }

    if (esp_partition_write(factoryPartition, written, buffer, readCount) !=
        ESP_OK) {
      httpClient.end();
      return false;
    }

    written += readCount;
    if (progressCallback && totalSize > 0) {
      progressCallback(static_cast<float>(written) /
                       static_cast<float>(totalSize));
    }
  }

  httpClient.end();

  if (written != static_cast<size_t>(totalSize)) {
    return false;
  }

  return esp_ota_set_boot_partition(factoryPartition) == ESP_OK;
}
#endif
