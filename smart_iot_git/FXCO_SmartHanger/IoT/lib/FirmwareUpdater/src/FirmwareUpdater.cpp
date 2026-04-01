/*
  [파일 설명] lib/FirmwareUpdater/src/FirmwareUpdater.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "FirmwareUpdater.h"

FirmwareUpdater::FirmwareUpdater(FirmwareInfo info)
    : firmwareIdentifier(info.uuid), currentFirmwareVersionNumber(info.version),
      autoReset(true) {}

/*
  performFirmwareUpdate()
  - 서버에 현재 버전을 전달해 업데이트 필요 여부를 확인한다.
  - 업데이트 데이터가 오면 chunk 단위로 플래시에 기록한다.
  - 진행률/결과를 콜백으로 상위 UI/로직에 전달한다.
*/
void FirmwareUpdater::performFirmwareUpdate(
    std::function<void(float)> progressCallback,
    std::function<void(FirmwareUpdateResult)> resultCallback) {
  FirmwareUpdateResult result = FirmwareUpdateResult::FAILED;

  // 1) 서버에 현재 버전을 전달하고 업데이트 파일 요청
  HTTPClient httpClient;
  httpClient.begin(firmwareServerUrl + "/firmwareDownload");
  httpClient.addHeader("Content-Type", "application/json");

  // 현재 버전을 전달해 서버가 업데이트 여부를 판단하도록 한다.
  int responseCode =
      httpClient.POST("{\"ProjectId\": \"" + firmwareIdentifier +
                      "\", \"currentVersion\": " +
                      String(currentFirmwareVersionNumber) + "}");

  if (responseCode == 201) {
    // 2) 서버가 바이너리 스트림을 내려주면 플래시에 기록
    WiFiClient *stream = httpClient.getStreamPtr();
    size_t totalSize = httpClient.getSize();
    size_t remainingSize = totalSize;

    if (totalSize > 0 && Update.begin(totalSize)) {
      size_t writtenSize = 0;
      float lastProgress = 0.0f;

      // 3) chunk 단위로 수신/기록 반복
      while (httpClient.connected() &&
             (remainingSize > 0 || remainingSize == (size_t)-1)) {
        size_t chunkSize = min((size_t)128, remainingSize);
        int bytesRead = stream->read(updateBuffer, chunkSize);

        if (bytesRead > 0) {
          writtenSize += Update.write(updateBuffer, bytesRead);

          // 콜백 과호출 방지를 위해 3% 이상 변할 때만 보고
          // (UI 갱신 과부하 방지)
          if (progressCallback) {
            float nowProgress =
                static_cast<float>(writtenSize) / static_cast<float>(totalSize);
            if (lastProgress + 0.03f <= nowProgress) {
              lastProgress = nowProgress;
              progressCallback(nowProgress);
            }
          }
        }

        if (remainingSize != (size_t)-1) {
          remainingSize -= bytesRead;
        }
      }

      // 4) 플래시 기록 완료/검증
      if (Update.end(true)) {
        result = FirmwareUpdateResult::SUCCESS;
      }
    }

  } else {
    // 서버 텍스트 메시지를 enum 실패 코드로 매핑
    String responseBody = httpClient.getString();

    if (responseBody.indexOf("프로젝트를 찾을 수 없습니다.") != -1)
      result = FirmwareUpdateResult::PROJECT_NOT_FOUND;
    else if (responseBody.indexOf("프로젝트 버전을 찾을 수 없습니다.") != -1)
      result = FirmwareUpdateResult::VERSION_NOT_FOUND;
    else if (responseBody.indexOf("현재 버전이 최신 버전입니다") != -1)
      result = FirmwareUpdateResult::NO_UPDATE_NEEDED;
    else if (responseBody.indexOf("bin 파일을 찾을 수 없습니다.") != -1)
      result = FirmwareUpdateResult::FILE_NOT_FOUND;
    else
      result = FirmwareUpdateResult::FAILED;
  }

  if (resultCallback) {
    resultCallback(result);
  }

  // 5) 성공 시 자동 재부팅 옵션 처리
  if (autoReset && result == FirmwareUpdateResult::SUCCESS) {
    reset();
  }
}

void FirmwareUpdater::setAutoReset(bool enable) { autoReset = enable; }

// reset(): 플랫폼에 맞는 재부팅 루틴 수행
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

