# stylingbooth SETUP_GUIDE

## 준비물
- Seeed XIAO ESP32-C3
- RFID Reader (코드 기준 FF704, UART)
- 상태 LED
- USB-C 케이블
- (선택) 문 센서/버튼 하드웨어

## 배선도(코드 기준)
- LED: `D4`
- RFID UART: `RX=D7`, `TX=D6`, `Serial1`, `115200`
- 센서 객체는 `D1`, `D3` 생성되어 있으나 main 로직에서 직접 사용은 제한적

## VS Code + PlatformIO 설치
1. VS Code 설치
2. PlatformIO IDE 설치
3. `stylingbooth` 폴더 열기

## 라이브러리 설치
- `platformio.ini` 자동 설치
  - `UIPEthernet`
  - `MySQL_Connector_Arduino`
  - `ArduinoJson`

## TFT_eSPI 설정
- 이 프로젝트는 TFT_eSPI를 사용하지 않습니다.

## 컴파일/업로드/시리얼 모니터
1. Build
2. Upload
3. Monitor(115200)

## 자주 막히는 문제와 해결
- door 이벤트가 안 들어옴
  - 구독 토픽 UUID 하드코딩 값 확인
  - 브로커 접속 계정/포트 확인
- RFID 태그가 누락됨
  - FF704 배선(RX/TX 반전 여부) 확인
  - baudrate 115200 확인
- 태그 전송 안 됨
  - 10초 스캔 만료 후에도 count>=5 태그가 없으면 전송 안 함(설계)
  - Door API URL/포트/ID 확인

## 검증 체크리스트
- door=open 입력 시 READY publish + openDoor 호출 확인
- door=closed 입력 시 SCANNING publish + RFID 시작 확인
- 약 10초 후 USING publish + tags POST 확인
- scan percent가 증가하며 publish되는지 확인
