#include "FirmwareUpdater.h"

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <Updater.h>
#else
#include <EEPROM.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#endif

namespace {
constexpr int kOtaMarkerEepromSize = 64;
constexpr int kOtaMarkerAddr = 16;
constexpr int kOtaMarkerLength = 16;

String normalizedMacAddress() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();
  return mac;
}

void persistOtaMarker(const char* value) {
  EEPROM.begin(kOtaMarkerEepromSize);
  for (int i = 0; i < kOtaMarkerLength; ++i) {
    const char c = (value && value[i] != '\0') ? value[i] : '\0';
    EEPROM.write(kOtaMarkerAddr + i, static_cast<uint8_t>(c));
  }
  EEPROM.commit();
}
}

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

void FirmwareUpdater::setBootSourceHint(const String& hint) { bootSourceHint = hint; }

void FirmwareUpdater::performFirmwareUpdate(
    std::function<void(float)> progressCallback,
    std::function<void(FirmwareUpdateResult)> resultCallback) {
  
  FirmwareUpdateResult result = FirmwareUpdateResult::FAILED;

  // 업데이트 중 와이파이 연결 유지 강화
  WiFi.setAutoReconnect(true);

  WiFiClient client;
  HTTPClient httpClient;
  
  String requestUrl = firmwareServerUrl + "/firmwareDownload";
  if (!httpClient.begin(client, requestUrl)) {
    Serial.println("[OTA] Failed to begin HTTP connection");
    if (resultCallback) resultCallback(result);
    return;
  }
  
  httpClient.setTimeout(10000); // 대용량 다운로드를 위해 타임아웃을 10초로 늘림
  httpClient.addHeader("Content-Type", "application/json");

  const String payload =
      "{\"projectId\": " + String(firmwareProjectId) +
      ", \"chipType\": \"" + currentChipType +
      "\", \"currentFirmwareFamily\": \"" + currentFirmwareFamily +
      "\", \"currentVersion\": " + String(currentFirmwareVersionNumber) +
      ", \"macAddress\": \"" + normalizedMacAddress() +
      "\", \"bootSourceHint\": \"" + bootSourceHint + "\"}";

  Serial.printf("[OTA] Requesting update from: %s\n", requestUrl.c_str());
  const int responseCode = httpClient.POST(payload);
  Serial.printf("[OTA] Response code: %d\n", responseCode);

  if (responseCode == 200 || responseCode == 201) {
    const String contentType = httpClient.header("Content-Type");
    
    // 1. JSON 응답인 경우 (업데이트 없음 또는 메시지)
    if (contentType.indexOf("application/json") != -1) {
      const String responseBody = httpClient.getString();
      Serial.printf("[OTA] Server Message: %s\n", responseBody.c_str());
      
      if (responseBody.indexOf("no newer firmware") != -1 || 
          responseBody.indexOf("업데이트가 없습니다") != -1) {
        result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
      }
    } 
    // 2. 바이너리 스트림인 경우 (실제 업데이트 시작)
    else {
      const size_t totalSize = static_cast<size_t>(httpClient.getSize());
      WiFiClient* stream = httpClient.getStreamPtr();

      if (totalSize > 0) {
        Serial.printf("[OTA] Starting Update. Size: %u bytes\n", (unsigned int)totalSize);
        
        // ESP32/8266 업데이트 시작
        if (Update.begin(totalSize)) {
          size_t written = 0;
          float lastProgress = 0.0f;
          uint8_t buffer[1024];

          while (httpClient.connected() && written < totalSize) {
            size_t available = stream->available();
            if (!available) {
              delay(1);
              continue;
            }

            size_t toRead = min(sizeof(buffer), min(available, totalSize - written));
            size_t readCount = stream->readBytes(buffer, toRead);
            
            if (readCount > 0) {
              size_t chunkWritten = Update.write(buffer, readCount);
              if (chunkWritten != readCount) {
                Serial.printf("[OTA Error] Write failed. Expected %u, wrote %u\n", readCount, chunkWritten);
                break;
              }
              written += chunkWritten;

              // 프로그레스 콜백
              if (progressCallback) {
                float progress = static_cast<float>(written) / static_cast<float>(totalSize);
                if ((progress - lastProgress) >= 0.03f || written == totalSize) {
                  lastProgress = progress;
                  progressCallback(progress);
                }
              }
            }
          }

          if (written == totalSize) {
            if (Update.end(true)) {
              Serial.println("[OTA] Update Success!");
              persistOtaMarker("ota_success");
              result = FirmwareUpdateResult::SUCCESS;
            } else {
              Serial.printf("[OTA Error] Update.end failed. Error #: %d\n", Update.getError());
            }
          }
        } else {
          Serial.printf("[OTA Error] Not enough space for update. Error #: %d\n", Update.getError());
        }
      }
    }
  } 
  // 3. 에러 발생 시 처리
  else {
    const String responseBody = httpClient.getString();
    Serial.printf("[OTA] Error Response: %s\n", responseBody.c_str());

    if (responseCode == 400 || responseBody.indexOf("no newer firmware") != -1) {
      result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
    } else if (responseBody.indexOf("project not found") != -1) {
      result = FirmwareUpdateResult::PROJECT_NOT_FOUND;
    } else if (responseBody.indexOf("version not found") != -1) {
      result = FirmwareUpdateResult::VERSION_NOT_FOUND;
    } else if (responseBody.indexOf("bin file not found") != -1) {
      result = FirmwareUpdateResult::FILE_NOT_FOUND;
    }
  }

  httpClient.end();

  if (resultCallback) {
    resultCallback(result);
  }

  // 성공 시 자동 재부팅
  if (autoReset && result == FirmwareUpdateResult::SUCCESS) {
    Serial.println("[OTA] Rebooting in 2 seconds...");
    delay(2000);
    reset();
  }
}

void FirmwareUpdater::setAutoReset(bool enable) { autoReset = enable; }

void FirmwareUpdater::reset() { 
    // 재부팅 전 와이파이 설정을 플래시에 저장하도록 강제하려면 여기에 추가 로직 가능
    ESP.restart(); 
}
