#include "FirmwareUpdater.h"

FirmwareUpdater::FirmwareUpdater(const String &url, int port,
                                 const String &uuid, int firmwareVersion)
    : firmwareServerUrl(url), firmwareIdentifier(uuid),
      currentFirmwareVersionNumber(firmwareVersion), autoReset(true),
      port(port) {}

void FirmwareUpdater::performFirmwareUpdate(
    std::function<void(float)> progressCallback,
    std::function<void(FirmwareUpdateResult)> resultCallback) {
  FirmwareUpdateResult result = FirmwareUpdateResult::FAILED;

  EthernetClient client;

  // 서버 주소/포트 분리
  String host = firmwareServerUrl;

  // 서버 접속
  if (client.connect(host.c_str(), port)) {
    // JSON Body 작성
    String body =
        "{\"ProjectId\": \"" + firmwareIdentifier +
        "\", \"currentVersion\": " + String(currentFirmwareVersionNumber) + "}";

    // HTTP POST 요청 작성
    String request = "POST /firmwareDownload HTTP/1.1\r\n"
                     "Host: " +
                     host +
                     "\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: " +
                     String(body.length()) +
                     "\r\n"
                     "Connection: close\r\n\r\n" +
                     body;

    client.print(request);

    // 응답 헤더 읽기
    String header = "";
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r")
        break; // 헤더 끝
      header += line + "\n";
    }

    // 상태 코드 파싱 (예: "HTTP/1.1 201 Created")
    if (header.indexOf("201") != -1) {
      // Content-Length 추출
      int contentLength = 0;
      int idx = header.indexOf("Content-Length: ");
      if (idx != -1) {
        int endIdx = header.indexOf('\n', idx);
        String lenStr = header.substring(idx + 16, endIdx);
        contentLength = lenStr.toInt();
      }

      if (contentLength > 0 && Update.begin(contentLength)) {
        size_t writtenSize = 0;
        size_t totalSize = contentLength;

        float progress = 0.0f;

        while (client.connected() && writtenSize < totalSize) {
          int bytesRead = client.read(updateBuffer, sizeof(updateBuffer));

          if (bytesRead > 0) {
            writtenSize += Update.write(updateBuffer, bytesRead);

            float nowProgress = round((static_cast<float>(writtenSize) /
                                       static_cast<float>(totalSize)) *
                                      100) /
                                100;
            nowProgress = constrain(nowProgress, 0.0f, 1.0f);

            if (progressCallback) {
              if (nowProgress != progress) {
                progressCallback(nowProgress);
                progress = nowProgress;
              }
            }
          }
        }

        if (Update.end(true)) {
          result = FirmwareUpdateResult::SUCCESS;
        }
      }
    } else {
      // 바디 읽기
      String responseBody = client.readString();

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

    client.stop();
  }

  if (resultCallback) {
    resultCallback(result);
  }

  if (autoReset && result == FirmwareUpdateResult::SUCCESS) {
    delay(1000);
    reset();
  }
}

void FirmwareUpdater::setAutoReset(bool enable) { autoReset = enable; }

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
