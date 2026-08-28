#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include "UIPEthernet.h"
#include <Arduino.h>
#include <Update.h>
#include <functional>
#include <cmath> 

enum class FirmwareUpdateResult {
  SUCCESS,           ///< 업데이트 성공
  PROJECT_NOT_FOUND, ///< 프로젝트를 찾을 수 없음
  VERSION_NOT_FOUND, ///< 버전을 찾을 수 없음
  NO_UPDATE_NEEDED,  ///< 최신 버전으로 업데이트가 필요 없음
  FILE_NOT_FOUND,    ///< 파일이 없음
  FAILED             ///< 업데이트 실패
};

class FirmwareUpdater {
public:
  FirmwareUpdater(const String &url, int port, const String &uuid,
                  int firmwareVersion);

  void performFirmwareUpdate(
      std::function<void(float)> progressCallback,
      std::function<void(FirmwareUpdateResult)> resultCallback);

  void setAutoReset(bool enable);

  void reset();

private:
  String firmwareServerUrl;         ///< 펌웨어 서버 URL
  String firmwareIdentifier;        ///< 펌웨어 고유 식별자
  int currentFirmwareVersionNumber; ///< 현재 펌웨어 버전
  uint8_t updateBuffer[128];        ///< 펌웨어 데이터 버퍼
  bool autoReset;                   ///< 자동 재부팅 활성화 여부 (기본값: true)
  int port;
};

#endif // FIRMWARE_UPDATER_H
