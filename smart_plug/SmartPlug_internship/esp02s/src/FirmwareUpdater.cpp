#include "FirmwareUpdater.h"

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <Updater.h>
#else
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#endif

#ifndef UPDATE_SIZE_UNKNOWN
#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF
#endif
namespace {
constexpr int kOtaMarkerEepromSize = 32;
constexpr int kOtaMarkerAddr = 16;
constexpr int kOtaMarkerLength = 16;

String normalizedMacAddress() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();
  return mac;
}

void persistOtaMarker(const char* value) {
#if defined(ESP8266)
  EEPROM.begin(kOtaMarkerEepromSize);
  for (int i = 0; i < kOtaMarkerLength; ++i) {
    const char c = (value && value[i] != '\0') ? value[i] : '\0';
    EEPROM.write(kOtaMarkerAddr + i, static_cast<uint8_t>(c));
  }
  EEPROM.commit();
#else
  (void)value;
#endif
}

size_t otaFreeSketchSpace() {
#if defined(ESP8266)
  return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#else
  return UPDATE_SIZE_UNKNOWN;
#endif
}
}  // namespace

FirmwareUpdater::FirmwareUpdater(const String& url, int projectId,
                                 int firmwareVersion,
                                 const String& chipType,
                                 const String& firmwareFamily)
    : firmwareServerUrl(url),
      firmwareProjectId(projectId),
      currentFirmwareVersionNumber(firmwareVersion),
      currentChipType(chipType),
      currentFirmwareFamily(firmwareFamily),
      bootSourceHint("stable"),
      autoReset(true) {}

void FirmwareUpdater::setDebugLogger(std::function<void(const String&)> logger) {
  debugLogger = std::move(logger);
}

void FirmwareUpdater::setBootSourceHint(const String& hint) {
  bootSourceHint = hint;
}

void FirmwareUpdater::debugLog(const String& message) {
  if (debugLogger) {
    debugLogger(message);
  }
}

void FirmwareUpdater::debugUpdateError(const char* context) {
#if defined(ESP8266)
  String message = "ota:";
  message += context ? context : "update_error";
  message += ":code=";
  message += String(Update.getError());
  message += ":msg=";
  message += Update.getErrorString();
  debugLog(message);
#else
  String message = "ota:";
  message += context ? context : "update_error";
  debugLog(message);
#endif
}

void FirmwareUpdater::performFirmwareUpdate(
    std::function<void(float)> progressCallback,
    std::function<void(FirmwareUpdateResult)> resultCallback,
    std::function<void()> tickCallback) {
  FirmwareUpdateResult result = FirmwareUpdateResult::FAILED;

  WiFiClient client;
  HTTPClient httpClient;
  if (!httpClient.begin(client, firmwareServerUrl + "/firmwareDownload")) {
    debugLog("ota:http_begin_failed");
    if (resultCallback) {
      resultCallback(result);
    }
    return;
  }

  httpClient.setTimeout(10000);
  httpClient.addHeader("Content-Type", "application/json");
  const String payload =
      "{\"projectId\": " + String(firmwareProjectId) +
      ", \"chipType\": \"" + currentChipType +
      "\", \"currentFirmwareFamily\": \"" + currentFirmwareFamily +
      "\", \"currentVersion\": " + String(currentFirmwareVersionNumber) +
      ", \"macAddress\": \"" + normalizedMacAddress() +
      "\", \"bootSourceHint\": \"" + bootSourceHint + "\"}";

  debugLog("ota:request_start");
  const int responseCode = httpClient.POST(payload);
  Serial.printf("[OTA] firmwareDownload response code: %d\n", responseCode);
  debugLog("ota:response_code=" + String(responseCode));

  if (responseCode == 200 || responseCode == 201) {
    const String contentType = httpClient.header("Content-Type");
    const String targetFamily = httpClient.header("X-Target-Firmware-Family");
    debugLog("ota:content_type=" + contentType);
    if (targetFamily.length()) {
      debugLog("ota:target_family=" + targetFamily);
    }

    if (contentType.indexOf("application/json") != -1) {
      const String responseBody = httpClient.getString();
      Serial.printf("[OTA] unexpected JSON body: %s\n", responseBody.c_str());
      debugLog("ota:unexpected_json");
      if (responseBody.indexOf("no newer firmware available in the same family.") != -1) {
        result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
      }
    } else {
      const int reportedSize = httpClient.getSize();
      const size_t totalSize = reportedSize > 0 ? static_cast<size_t>(reportedSize) : 0;
      const size_t freeSpace = otaFreeSketchSpace();
      WiFiClient* stream = httpClient.getStreamPtr();

      debugLog("ota:file_size=" + String(static_cast<unsigned long>(totalSize)));
      debugLog("ota:free_space=" + String(static_cast<unsigned long>(freeSpace)));
#if defined(ESP8266)
      debugLog("ota:flash_real=" + String(ESP.getFlashChipRealSize()));
      debugLog("ota:flash_sdk=" + String(ESP.getFlashChipSize()));
#endif

      if (totalSize > 0 && freeSpace > 0 && totalSize > freeSpace) {
        debugLog("ota:not_enough_space");
      } else {
        const size_t updateSize = totalSize > 0 ? totalSize : UPDATE_SIZE_UNKNOWN;
        if (Update.begin(updateSize)) {
          size_t written = 0;
          int lastProgressBucket = -1;
          uint8_t buffer[1024];
          bool downloadOk = true;
          unsigned long lastDataMs = millis();
          debugLog("ota:update_begin_ok");

          while ((httpClient.connected() || stream->available()) &&
                 ((totalSize > 0 && written < totalSize) || totalSize == 0)) {
            yield();
            if (tickCallback) {
              tickCallback();
            }

            const size_t available = stream->available();
            if (!available) {
              if (millis() - lastDataMs > 10000UL) {
                debugLog("ota:download_timeout");
                downloadOk = false;
                break;
              }
              delay(1);
              continue;
            }
            lastDataMs = millis();

            const size_t remaining = totalSize > 0 ? (totalSize - written) : sizeof(buffer);
            const size_t toRead = min(sizeof(buffer), min(available, remaining));
            const size_t readCount = stream->readBytes(buffer, toRead);
            if (readCount == 0) {
              continue;
            }

            const size_t chunkWritten = Update.write(buffer, readCount);
            if (chunkWritten != readCount) {
              Serial.printf("[OTA Error] Update.write failed: expected %u, wrote %u\n",
                            static_cast<unsigned>(readCount),
                            static_cast<unsigned>(chunkWritten));
              debugLog("ota:update_write_failed");
              downloadOk = false;
              break;
            }

            written += chunkWritten;

            if (progressCallback && totalSize > 0) {
              const float progress = static_cast<float>(written) / static_cast<float>(totalSize);
              const int percent = static_cast<int>(progress * 100.0f);
              const int progressBucket = percent / 25;
              if (progressBucket != lastProgressBucket || written == totalSize) {
                lastProgressBucket = progressBucket;
                progressCallback(progress);
                debugLog("ota:progress=" + String(percent));
              }
            }
          }

          debugLog("ota:written=" + String(static_cast<unsigned long>(written)));

          if (downloadOk) {
            if (totalSize > 0 && written != totalSize) {
              debugLog("ota:size_mismatch");
            } else if (Update.end(true)) {
              result = FirmwareUpdateResult::SUCCESS;
              debugLog("ota:update_success");
              persistOtaMarker("ota_success");
            } else {
#if defined(ESP8266)
              Serial.printf("[OTA Error] Update.end failed. Error #: %d\n", Update.getError());
              debugUpdateError("update_end_failed");
#else
              Serial.println("[OTA Error] Update.end failed.");
              debugLog("ota:update_end_failed");
#endif
            }
          }
        } else {
          Serial.println("[OTA Error] Update.begin failed.");
          debugUpdateError("update_begin_failed");
        }
      }
    }
  } else if (responseCode == 400) {
    result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
    debugLog("ota:no_update_needed");
  } else {
    const String responseBody = httpClient.getString();
    Serial.printf("[OTA] firmwareDownload response body: %s\n", responseBody.c_str());
    debugLog("ota:error_body");
    if (responseBody.indexOf("project not found.") != -1) {
      result = FirmwareUpdateResult::PROJECT_NOT_FOUND;
    } else if (responseBody.indexOf("project version not found.") != -1 ||
               responseBody.indexOf("matching chipType version not found.") != -1) {
      result = FirmwareUpdateResult::VERSION_NOT_FOUND;
    } else if (responseBody.indexOf("no newer firmware available in the same family.") != -1) {
      result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
    } else if (responseBody.indexOf("bin file not found.") != -1) {
      result = FirmwareUpdateResult::FILE_NOT_FOUND;
    }
  }

  httpClient.end();

  if (resultCallback) {
    resultCallback(result);
  }

  if (autoReset && result == FirmwareUpdateResult::SUCCESS) {
    debugLog("ota:restart");
    reset();
  }
}

void FirmwareUpdater::setAutoReset(bool enable) { autoReset = enable; }

void FirmwareUpdater::reset() { ESP.restart(); }
