#include "FirmwareUpdater.h"

/**
 * @brief FirmwareUpdater 객체를 생성합니다.
 * @param url 펌웨어 서버 URL
 * @param uuid 펌웨어 고유 식별자
 * @param firmwareVersion 현재 펌웨어 버전
 */
FirmwareUpdater::FirmwareUpdater(const String &url, const String &uuid,
                                 int firmwareVersion)
    : firmwareServerUrl(url), firmwareIdentifier(uuid),
      currentFirmwareVersionNumber(firmwareVersion), autoReset(true) {}

/**
 * @brief 펌웨어 업데이트를 수행합니다.
 * @param progressCallback 업데이트 진행 상황을 보고하는 콜백 함수
 * @param resultCallback 업데이트 결과를 보고하는 콜백 함수
 */
void FirmwareUpdater::performFirmwareUpdate(
    std::function<void(float)> progressCallback,
    std::function<void(FirmwareUpdateResult)> resultCallback) {
  FirmwareUpdateResult result = FirmwareUpdateResult::FAILED;

  HTTPClient httpClient;
  httpClient.begin(firmwareServerUrl + "/firmwareDownload");
  httpClient.addHeader("Content-Type", "application/json");
  int responseCode = httpClient.POST(
      "{\"ProjectId\": \"" + firmwareIdentifier +
      "\", \"currentVersion\": " + String(currentFirmwareVersionNumber) + "}");

  if (responseCode == 201) {
    String responseBody = httpClient.getString();

    WiFiClient *stream = httpClient.getStreamPtr();
    size_t totalSize = httpClient.getSize();
    size_t remainingSize = totalSize;

    if (totalSize > 0 && Update.begin(totalSize)) {
      size_t writtenSize = 0;

      float lastProgress = 0.0f;
      while (httpClient.connected() &&
             (remainingSize > 0 || remainingSize == -1)) {
        size_t chunkSize = min((size_t)128, remainingSize);
        int bytesRead = stream->read(updateBuffer, chunkSize);
        if (bytesRead > 0) {
          writtenSize += Update.write(updateBuffer, bytesRead);
          if (progressCallback) {
            float nowProgress =
                static_cast<float>(writtenSize) / static_cast<float>(totalSize);
            if (lastProgress + 0.03 <= nowProgress) {
              lastProgress = nowProgress;
              progressCallback(static_cast<float>(writtenSize) /
                               static_cast<float>(totalSize));
            }
          }
        }
        remainingSize -= bytesRead;
      }

      if (Update.end(true)) {
        result = FirmwareUpdateResult::SUCCESS;
      }
    }
  } else {
    String responseBody = httpClient.getString();
    if (responseBody.indexOf("프로젝트를 찾을 수 없습니다.") != -1)
      result = FirmwareUpdateResult::PROJECT_NOT_FOUND;
    else if (responseBody.indexOf("프로젝트 버전을 찾을 수 없습니다.") != -1)
      result = FirmwareUpdateResult::VERSION_NOT_FOUND;
    else if (responseBody.indexOf("현재 버전이 최신 버전입니다.") != -1)
      result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
    else if (responseBody.indexOf("bin 파일을 찾을 수 없습니다.") != -1)
      result = FirmwareUpdateResult::FILE_NOT_FOUND;
    else
      result = FirmwareUpdateResult::FAILED;
  }

  if (resultCallback) {
    resultCallback(result);
  }

  if (autoReset && result == FirmwareUpdateResult::SUCCESS) {
    reset();
  }
}

/**
 * @brief 자동 재부팅 기능을 설정합니다.
 * @param enable 자동 재부팅을 활성화할지 여부 (기본값: true)
 */
void FirmwareUpdater::setAutoReset(bool enable) { autoReset = enable; }

/**
 * @brief 시스템을 재부팅합니다.
 */
void FirmwareUpdater::reset() {
#ifdef ESP_PLATFORM
  ESP.restart();
#elif defined(ARDUINO) && !defined(ESP_PLATFORM)
  wdt_enable(WDTO_15MS);
  while (true)
    ;
#elif defined(LINUX)
  system("reboot");
#endif
}

/**
 * @brief 현재 펌웨어 버전 번호를 가져옵니다.
 * @return 현재 펌웨어 버전 번호
 */
int FirmwareUpdater::getFirmwareVersionNumber() {
  return currentFirmwareVersionNumber;
}