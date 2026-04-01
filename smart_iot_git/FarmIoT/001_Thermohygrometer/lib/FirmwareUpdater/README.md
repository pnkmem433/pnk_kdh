# FirmwareUpdater

FirmwareUpdater 라이브러리는 NestJS 서버와 연결하여 파일을 다운로드하고 펌웨어를 업그레이드하는 데 도움을 줍니다. 이 라이브러리를 사용하면 펌웨어 업데이트 과정이 간편해지고, 서버와의 연결을 통해 쉽게 파일을 다운로드할 수 있습니다.

## 설치 방법

FirmwareUpdater 라이브러리를 설치하려면, `FirmwareUpdater` 폴더를 Arduino 라이브러리 디렉토리에 복사하세요.

## 사용 방법

### `FirmwareUpdater` 클래스

- `FirmwareUpdater(const String& url, const String& uuid, int firmwareVersion)`

  `FirmwareUpdater` 객체를 생성합니다.

  - `url`: 펌웨어 서버 URL
  - `uuid`: 펌웨어 고유 식별자
  - `firmwareVersion`: 현재 펌웨어 버전

- `void performFirmwareUpdate(std::function<void(int, int, UpdateDirection)> progressCallback, std::function<void(FirmwareUpdateResult)> resultCallback)`

  펌웨어 업데이트를 수행합니다.

  - `progressCallback`: 업데이트 진행 상황을 보고하는 콜백 함수
  - `resultCallback`: 업데이트 결과를 보고하는 콜백 함수

- `void setAutoReset(bool enable)`

  자동 재부팅 기능을 설정합니다. (기본값: true)

  - `enable`: 자동 재부팅을 활성화할지 여부

- `void reset()`

  시스템을 재부팅합니다.

## 예제

`examples/SimpleUpdate/SimpleUpdate.ino` 파일을 열어, 라이브러리를 사용하는 방법을 확인하세요.

## 라이센스

이 라이브러리는 [MIT 라이센스](https://opensource.org/licenses/MIT) 하에 제공됩니다.
