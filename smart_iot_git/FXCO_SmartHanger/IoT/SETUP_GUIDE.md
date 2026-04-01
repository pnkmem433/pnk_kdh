# Smart Hanger 시작 가이드 (초보자용)

이 문서는 **처음 보는 사람** 기준으로 작성했습니다.

## 1) 준비물
- 보드: Seeed Studio XIAO ESP32C3
- NFC 리더: MFRC522
- TFT: GC9A01 240x240 원형 LCD (XIAO용 쉴드 결합 방식)
- 점퍼선, USB-C 케이블
- PC: Windows + VS Code
- PlatformIO 확장

## 1-1) UUID를 왜 쓰나요?
- 행거마다 **고유 번호**가 있어야 서버가 장치를 구분할 수 있습니다.
- 이 프로젝트는 UUID를 EEPROM에 저장해서 전원을 껐다 켜도 같은 장치로 인식됩니다.
- `connect/pickup/pickdown` 같은 이벤트가 어떤 행거에서 발생했는지 식별하는 기준값입니다.
- 현장 운영에서 로그 추적/장애 분석/장치 교체 이력 관리에 필수입니다.
- OTA 업데이트 운영에도 필수입니다.
  - UUID가 없으면 장치별 번호를 수기로 관리하며 업데이트 대상을 하나씩 맞춰야 합니다.
  - UUID를 장치가 랜덤 생성 후 유지하면 같은 펌웨어를 공통 배포해도 서버에서 장치별 관리가 자동화되어 운영 부담이 크게 줄어듭니다.

## 1-2) EEPROM은 왜 쓰나요?
- UUID를 재부팅 후에도 유지하려고 사용합니다.
- RAM에만 저장하면 전원 OFF/재부팅 때 UUID가 사라집니다.
- EEPROM은 전원이 꺼져도 값이 남는 비휘발성 메모리라서, UUID를 한 번 저장하면 계속 재사용할 수 있습니다.
- 그래서 장치 교체 전까지 같은 UUID를 유지하며 서버/MQTT/OTA가 안정적으로 장치를 식별합니다.
- SPIFFS/LittleFS처럼 파일시스템을 쓰면 쓰기 중 전원 차단 시 파일 오류 가능성이 있습니다.
- UUID처럼 작은 값 1개 저장 목적에서는 EEPROM 방식이 더 단순하고 안정적으로 운영하기 쉽습니다.

## 2) 배선도 (핵심)

### 2-1. NFC RC522 배선
- `SS(SDA)` -> `D4`
- `RST` -> `D2`
- `SCK` -> `D8`
- `MISO` -> `D9`
- `MOSI` -> `D10`
- `VCC` -> `3.3V`
- `GND` -> `GND`

### 2-2. TFT GC9A01 배선
- TFT는 점퍼 배선이 아니라 **쉴드 결합(스택)** 기준입니다.
- 즉, 보드와 TFT 쉴드를 결합하면 TFT 핀은 하드웨어적으로 연결됩니다.
- 이 프로젝트의 논리 핀 매핑은 `D8/D9/D10/D1/D3` 기준으로 동작합니다.

### 2-3. 충전 감지 입력
- `Sensor` 입력 핀: `D6`
- 이 프로젝트 기준 의미:
  - `HIGH` = 충전 중
  - `LOW` = 비충전

## 3) 개발환경 설치 (VS Code + PlatformIO부터)

### 3-1. VS Code 설치
1. VS Code를 설치하고 실행합니다.

### 3-2. VS Code에 PlatformIO 설치
1. VS Code 왼쪽 메뉴에서 `Extensions`를 엽니다.
2. 검색창에 `PlatformIO IDE`를 입력합니다.
3. 게시자가 `PlatformIO`인 확장을 선택해 `Install` 클릭합니다.
4. 설치가 끝나면 VS Code를 재시작합니다.
5. `Ctrl+Shift+P` -> `PlatformIO: Home` 실행해 정상 동작을 확인합니다.

### 3-3. 프로젝트 열기
1. 이 폴더(`IoT`)를 VS Code로 엽니다.
2. 하단 PlatformIO 메뉴에서 Build/Upload/Monitor 버튼이 보이는지 확인합니다.

### 3-4. 라이브러리 설치
이 프로젝트는 `platformio.ini`의 `lib_deps`를 통해 자동 설치됩니다.

## 4) TFT_eSPI 설정법 (중요)

이 프로젝트는 **`User_Setup_Select.h`에서 보드용 Setup을 선택**해서 맞추는 방식을 기준으로 합니다.

설정 순서:
1. Arduino 라이브러리 폴더의 `TFT_eSPI/User_Setup_Select.h` 열기
2. XIAO ESP32C3 + GC9A01 쉴드에 맞는 Setup 파일 1개만 활성화
3. 다른 Setup define은 주석 처리

필수 확인 항목:
- 드라이버: `GC9A01`
- 해상도: `240x240`
- 핀 맵: 쉴드 스펙과 일치
- SPI 속도: 화면 깨짐이 있으면 낮춰서 테스트

주의:
- `User_Setup_Select` 방식과 `platformio.ini build_flags` 강제 방식을 혼용하면 충돌할 수 있습니다.
- 한 가지 방식으로 통일하세요.

## 5) 네트워크/서버 값 점검
`src/main.cpp`에 아래 값들이 하드코딩되어 있습니다.
- Wi-Fi SSID/PW
- HTTP 서버 주소/포트
- MQTT 서버 주소/포트/계정
- 펌웨어 업데이트 서버는 `lib/FirmwareUpdater` 내부

실사용 전 반드시 현장 환경 값으로 바꾸세요.

## 6) 컴파일/업로드

### 6-1. VS Code 버튼 사용
- 하단 PlatformIO의
  - `Build`(체크 아이콘)
  - `Upload`(화살표 아이콘)
  - `Monitor`(시리얼)

### 6-2. CLI 사용
```bash
platformio run
platformio run -t upload
platformio device monitor -b 115200
```

## 7) 첫 부팅에서 보이는 정상 동작
1. 로고 표시
2. Wi-Fi 연결
3. OTA 확인
4. 서버 connect
5. 의류 데이터 없으면 QR 안내 화면 유지
6. 의류 데이터 있으면 QR/상품 화면 교차 표시
7. NFC 태그 올리면 pickdown 이벤트 전송
8. 태그 제거하면 pickup 이벤트 전송

## 8) 자주 막히는 포인트
- 빌드 실패: PlatformIO/툴체인 설치 누락
- TFT 안나옴: `User_Setup_Select` 선택 오류 또는 쉴드 결합 불량
- NFC 불안정: 전원 품질, SPI 공유 충돌, 배선 길이
- MQTT 연결 안됨: 서버 URI/포트/계정 오입력
- HTTP 응답 없음: 서버 주소/포트/방화벽 확인

## 9) 권장 체크리스트
- 배선 완료 후 `GND 공통` 먼저 확인
- 시리얼 로그(115200)로 부팅 단계 확인
- MQTT 브로커에서 토픽 수신 확인
- NFC Scan/Remove 2가지 모두 테스트
