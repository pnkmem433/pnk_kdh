#include "FirmwareUpdater.h"

#include <ESP8266WiFi.h>
#include <Updater.h>

namespace {

constexpr char kFirmwarePath[] = "/firmwareDownload";

}  // namespace

FirmwareUpdater::FirmwareUpdater(const char* serverUrl, const char* projectId, int versionCode)
    : serverUrl_(serverUrl), projectId_(projectId), versionCode_(versionCode) {}

String FirmwareUpdater::requestUrl() const {
  if (serverUrl_ == nullptr) {
    return String();
  }

  String url(serverUrl_);
  if (!url.endsWith(kFirmwarePath)) {
    if (url.endsWith("/")) {
      url.remove(url.length() - 1);
    }
    url += kFirmwarePath;
  }
  return url;
}

void FirmwareUpdater::performFirmwareUpdate(
    std::function<void(float)> progressCallback,
    std::function<void(FirmwareUpdateResult)> resultCallback) {
  String url = requestUrl();
  if (WiFi.status() != WL_CONNECTED || url.isEmpty()) {
    if (resultCallback) {
      resultCallback(FirmwareUpdateResult::Failed);
    }
    return;
  }

  FirmwareUpdateResult result = FirmwareUpdateResult::Failed;
  HTTPClient httpClient;
  WiFiClient wifiClient;

  if (!httpClient.begin(wifiClient, url)) {
    if (resultCallback) {
      resultCallback(result);
    }
    return;
  }

  httpClient.addHeader("Content-Type", "application/json");

  String requestBody = "{\"ProjectId\":\"";
  requestBody += projectId_ ? projectId_ : "";
  requestBody += "\",\"currentVersion\":";
  requestBody += String(versionCode_);
  requestBody += "}";

  int responseCode = httpClient.POST(requestBody);

  if (responseCode == HTTP_CODE_CREATED) {
    int totalSize = httpClient.getSize();
    WiFiClient* stream = httpClient.getStreamPtr();

    if (totalSize > 0 && Update.begin(static_cast<size_t>(totalSize))) {
      size_t writtenSize = 0;
      float lastProgress = 0.0f;

      while (httpClient.connected() && (writtenSize < static_cast<size_t>(totalSize))) {
        size_t available = stream->available();
        if (available == 0) {
          delay(1);
          yield();
          continue;
        }

        size_t chunkSize = available > sizeof(updateBuffer_) ? sizeof(updateBuffer_) : available;
        int bytesRead = stream->readBytes(updateBuffer_, chunkSize);
        if (bytesRead <= 0) {
          continue;
        }

        size_t bytesWritten = Update.write(updateBuffer_, static_cast<size_t>(bytesRead));
        writtenSize += bytesWritten;

        if (progressCallback && totalSize > 0) {
          float progress = static_cast<float>(writtenSize) / static_cast<float>(totalSize);
          if ((progress - lastProgress) >= 0.03f || writtenSize == static_cast<size_t>(totalSize)) {
            lastProgress = progress;
            progressCallback(progress);
          }
        }
      }

      if (writtenSize == static_cast<size_t>(totalSize) && Update.end(true)) {
        result = FirmwareUpdateResult::Success;
      }
    }
  } else {
    String responseBody = httpClient.getString();
    if (responseBody.indexOf("프로젝트를 찾을 수 없습니다.") != -1) {
      result = FirmwareUpdateResult::ProjectNotFound;
    } else if (responseBody.indexOf("프로젝트 버전을 찾을 수 없습니다.") != -1) {
      result = FirmwareUpdateResult::VersionNotFound;
    } else if (responseBody.indexOf("현재 버전이 최신 버전입니다.") != -1) {
      result = FirmwareUpdateResult::NoUpdate;
    } else if (responseBody.indexOf("bin 파일을 찾을 수 없습니다.") != -1) {
      result = FirmwareUpdateResult::FileNotFound;
    }
  }

  httpClient.end();

  if (resultCallback) {
    resultCallback(result);
  }

  if (autoReset_ && result == FirmwareUpdateResult::Success) {
    reset();
  }
}

void FirmwareUpdater::setAutoReset(bool enable) { autoReset_ = enable; }

void FirmwareUpdater::reset() {
  ESP.restart();
}
