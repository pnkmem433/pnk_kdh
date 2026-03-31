#include "FirmwareUpdater.h"

#include <ESP8266WiFi.h>

namespace {

constexpr char kFirmwarePath[] = "/firmwareDownload";

}  // namespace

FirmwareUpdater::FirmwareUpdater(const char* serverUrl) : serverUrl_(serverUrl) {}

String FirmwareUpdater::updateUrl() const {
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
  String url = updateUrl();
  if (WiFi.status() != WL_CONNECTED || url.isEmpty()) {
    if (resultCallback) {
      resultCallback(FirmwareUpdateResult::Failed);
    }
    return;
  }

  ESPhttpUpdate.onProgress([&progressCallback](int current, int total) {
    if (progressCallback && total > 0) {
      progressCallback(static_cast<float>(current) / static_cast<float>(total));
    }
  });

  WiFiClient wifiClient;
  t_httpUpdate_return result = ESPhttpUpdate.update(wifiClient, url);

  if (!resultCallback) {
    return;
  }

  switch (result) {
    case HTTP_UPDATE_OK:
      resultCallback(FirmwareUpdateResult::Success);
      break;
    case HTTP_UPDATE_NO_UPDATES:
      resultCallback(FirmwareUpdateResult::NoUpdate);
      break;
    default:
      resultCallback(FirmwareUpdateResult::Failed);
      break;
  }
}
