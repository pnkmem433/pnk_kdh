#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <functional>

/*
  ==================================================================================
  FirmwareUpdater 상세 설명
  ==================================================================================
  [동작 개요]
  1) 현재 펌웨어 버전과 프로젝트 ID를 서버로 전달
  2) 서버가 새 바이너리를 반환하면 스트리밍 수신
  3) Update API로 플래시에 기록
  4) 성공 시 결과 콜백 호출 후(옵션) 자동 재부팅

  [설계 의도]
  - 부팅 시 자동 업데이트를 지원해 현장 유지보수 비용을 낮춘다.
  - progressCallback을 통해 UI에서 진행률 표시 가능
  - 결과 enum으로 실패 원인을 상위 코드에 전달 가능
  ==================================================================================
*/

enum class FirmwareUpdateResult {
  SUCCESS,
  PROJECT_NOT_FOUND,
  VERSION_NOT_FOUND,
  NO_UPDATE_NEEDED,
  FILE_NOT_FOUND,
  FAILED
};

struct FirmwareInfo {
  String uuid;
  int version;
};

class FirmwareUpdater {
public:
  explicit FirmwareUpdater(FirmwareInfo info);

  void performFirmwareUpdate(
      std::function<void(float)> progressCallback,
      std::function<void(FirmwareUpdateResult)> resultCallback);

  void setAutoReset(bool enable);
  void reset();

private:
  String firmwareServerUrl = "http://gym907-0001.iptime.org:3004";
  String firmwareIdentifier;
  int currentFirmwareVersionNumber;
  uint8_t updateBuffer[128];
  bool autoReset;
};

#endif
