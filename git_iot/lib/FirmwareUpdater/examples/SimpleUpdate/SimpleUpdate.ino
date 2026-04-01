#include <WiFi.h>
#include <FirmwareUpdater.h>

// WiFi 네트워크의 SSID와 비밀번호
#define SSID ""
#define PASSWORD ""


#define SERVER "http://yourServerHost:port"  // 펌웨어 파일을 제공하는 서버의 주소
#define FIRMWARE_UUID ""                     // 업데이트할 펌웨어의 고유 식별자
#define CURRENT_VERSION 1                    // 현재 사용 중인 펌웨어 버전

// FirmwareUpdater 객체 생성
FirmwareUpdater updater(SERVER, FIRMWARE_UUID, CURRENT_VERSION);

void setup() {
  // 시리얼 통신 시작
  Serial.begin(115200);

  // WiFi 네트워크에 연결
  WiFi.begin(SSID, PASSWORD);
  // WiFi 연결이 완료될 때까지 대기
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // 자동 리셋 기능을 사용할지 여부 설정
  // 자동리셋은 펌웨어 업데이트가 성공했을때만 작동합니다.
  // 기본은 true이고 필요하지 않으시면 아래 주석을 해제해주세요.
  // updater.setAutoReset(false);

  // 펌웨어 업데이트 수행
  updater.performFirmwareUpdate(
    // 업데이트 진행 상태를 나타내는 콜백 함수
    [](float progress) {
      Serial.printf("펌웨어 적용 진행 중 : %.2f%%\n", percentage * 100);
    },
    // 업데이트 결과를 처리하는 콜백 함수
    [](FirmwareUpdateResult result) {
      switch (result) {
        case FirmwareUpdateResult::SUCCESS:
          Serial.println("업데이트 성공!");
          break;
        case FirmwareUpdateResult::PROJECT_NOT_FOUND:
          Serial.println("프로젝트를 찾을 수 없음.");
          break;
        case FirmwareUpdateResult::VERSION_NOT_FOUND:
          Serial.println("버전을 찾을 수 없음.");
          break;
        case FirmwareUpdateResult::NO_UPDATE_NEEDED:
          Serial.println("최신 버전입니다.");
          break;
        case FirmwareUpdateResult::FILE_NOT_FOUND:
          Serial.println("파일을 찾을 수 없음.");
          break;
        case FirmwareUpdateResult::FAILED:
          Serial.println("업데이트 실패.");
          break;
      }
    });

  Serial.printf("현재 펌웨어 버전은 %d 입니다.", CURRENT_VERSION);
}

void loop() {
  // 메인 루프는 비어 있음
}
