#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <functional>

/**
 * @brief 펌웨어 업데이트 결과를 나타내는 열거형입니다.
 */
enum class FirmwareUpdateResult {
  SUCCESS,          ///< 업데이트 성공
  PROJECT_NOT_FOUND,///< 프로젝트를 찾을 수 없음
  VERSION_NOT_FOUND,///< 버전을 찾을 수 없음
  NO_UPDATE_NEEDED, ///< 최신 버전으로 업데이트가 필요 없음
  FILE_NOT_FOUND,   ///< 파일이 없음
  FAILED            ///< 업데이트 실패
};

/**
 * @class FirmwareUpdater
 * @brief 펌웨어 업데이트를 처리하는 클래스입니다.
 */
class FirmwareUpdater {
public:
  /**
   * @brief FirmwareUpdater 객체를 생성합니다.
   * @param url 펌웨어 서버 URL
   * @param uuid 펌웨어 고유 식별자
   * @param firmwareVersion 현재 펌웨어 버전
   */
  FirmwareUpdater(const String& url, const String& uuid, int firmwareVersion);

  /**
   * @brief 펌웨어 업데이트를 수행합니다.
   * @param progressCallback 업데이트 진행 상황을 보고하는 콜백 함수
   * @param resultCallback 업데이트 결과를 보고하는 콜백 함수
   */
  void performFirmwareUpdate(std::function<void(float)> progressCallback,
                             std::function<void(FirmwareUpdateResult)> resultCallback);

  /**
   * @brief 자동 재부팅 기능을 설정합니다.
   * @param enable 자동 재부팅을 활성화할지 여부 (기본값: true)
   */
  void setAutoReset(bool enable);

  /**
   * @brief 시스템을 재부팅합니다.
   */
  void reset();
  
  /**
   * @brief 현재 펌웨어 버전 번호를 가져옵니다.
   * @return 현재 펌웨어 버전 번호
   */
  int getFirmwareVersionNumber();

private:
  String firmwareServerUrl;          ///< 펌웨어 서버 URL
  String firmwareIdentifier;         ///< 펌웨어 고유 식별자
  int currentFirmwareVersionNumber;  ///< 현재 펌웨어 버전
  uint8_t updateBuffer[128];         ///< 펌웨어 데이터 버퍼
  bool autoReset;                    ///< 자동 재부팅 활성화 여부 (기본값: true)
};

#endif // FIRMWARE_UPDATER_H
