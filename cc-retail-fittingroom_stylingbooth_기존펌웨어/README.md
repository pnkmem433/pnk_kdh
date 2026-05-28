# stylingbooth

## 프로젝트 목적
- 문 상태(MQTT)를 기준으로 RFID 스캔을 제어하고,
  스캔 결과를 HTTP Door API로 전송하는 피팅룸 오케스트레이션 펌웨어입니다.

## 전체 아키텍처
- `src/main.cpp`: 상태전이 오케스트레이션(door open/closed -> RFID/LED/API)
- `lib/mqtt`: ESP-IDF MQTT 래퍼(QoS1 재시도 포함)
- `lib/rfidReader`: FF701/FF704 리더 구현
- `lib/http`, `lib/doorApi`: 백엔드 API 호출
- `lib/led`: 상태 LED
- `lib/sensor`: 디지털 센서 이벤트 큐 처리
- `lib/lan`: 유선 Ethernet 유틸(현재 main에서 직접 사용 안 함)

## 모듈별 책임
- `Mqtt`: door 토픽 수신, LED strip 명령 토픽 발행
- `RfidReader`: 태그 읽기/누적 카운트
- `DoorApi`: 문 상태 PATCH, 태그 POST
- `LED`: 장치 상태 시각화

## 부팅 순서(setup)
1. LED 시작 및 초기 blink
2. RFID 콜백 등록
3. WiFi 연결
4. MQTT begin/connect 및 door 토픽 구독
5. HTTP/DoorApi/RFID begin
6. fittingRoomId 설정
7. DoorSensorTask 시작(진행률/완료 처리)

## 런타임 루프(loop)
- 메인 loop는 `delay(10)`만 수행
- 실질 로직은 MQTT 콜백과 DoorSensorTask에서 실행

## 이벤트 파이프라인
- door=open 수신
  - RFID stop + tag clear
  - LED strip mode=`READY` publish
  - DoorApi `openDoor()`
- door=closed 수신
  - RFID start
  - `lastCloseTime` 기록
  - LED strip mode=`SCANNING` publish
  - DoorApi `closeDoor()`
- 스캔 10초 만료
  - RFID stop
  - 태그 카운트 5회 이상만 필터
  - tag substring(5,13) 추출 후 DoorApi `sendTags()`
  - LED strip mode=`USING` publish

## MQTT 명세
- Subscribe
  - `fittingroom/door/9169A97A-96DE-4FFF-9167-A2B398C6C900/door`
  - payload: `open|closed`
- Publish
  - `fittingroom/led_strip/9642C3BA-FC4A-4B07-A0E0-9153D323EC06/command/mode`
    - `READY|SCANNING|USING`
  - `fittingroom/led_strip/9642C3BA-FC4A-4B07-A0E0-9153D323EC06/command/scan/percent`

## HTTP 명세
- `PATCH /fittingrooms/{id}/door` with `{"isClosed": true|false}`
- `POST /fittingrooms/{id}/tags` with `{"tags": ["..."]}`
- 타임아웃: 8000ms

## OTA 동작
- 이 프로젝트 코드에는 OTA 모듈이 직접 포함되어 있지 않습니다.

## UUID/EEPROM/NVS
- 이 프로젝트는 UUID/EEPROM을 현재 사용하지 않습니다(토픽이 하드코딩됨).
- 운영 확장 시 UUID 영속화를 도입하면 다수 장비 배포 시 토픽 충돌 방지에 유리합니다.

## SPIFFS/LittleFS 미사용 이유
- 현재는 파일 영속 데이터가 없어 파일시스템이 필요 없습니다.
- 전원 차단 중 파일 쓰기 리스크를 회피하려면 필요한 최소 영속 데이터만 EEPROM/NVS에 저장하는 편이 안전합니다.

## 전원 차단/재부팅 후 데이터 유지 전략
- 유지: 없음(RAM 기반 상태 초기화)
- 재동기화: door 토픽 이벤트를 다시 수신하면 상태 재구성

## 장애 대응 포인트
- RFID 노이즈는 태그 카운트 임계값(>=5)로 완화
- 진행률은 값 변화 시에만 publish해 트래픽 절감
- WiFi 자동 재연결 활성화

## build_flags 설정 규칙
- 이 프로젝트는 WiFi/MQTT/API 서버 값을 `platformio.ini`의 `build_flags`로 주입합니다.
- 기본값이 없고 필수 매크로 누락 시 `#error`로 컴파일 실패하도록 구성되어 있습니다.

### 필수 build_flags(요약)
- `WIFI_SSID`, `WIFI_PASS`
- `MQTT_HOST`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASS`
- `API_HOST`, `API_PORT`

### 주의
- `build_flags`는 런타임이 아니라 빌드 타임 설정입니다.
- 값 변경 후에는 반드시 재빌드/재업로드해야 반영됩니다.

## 운영 체크리스트
- door/led 토픽 하드코딩 UUID가 서버 설정과 일치하는지 확인
- `fittingRoomId=1`이 백엔드 설정과 일치하는지 확인
- FF704 RX/TX/baudrate(115200) 배선 확인
- 스캔 윈도우(10초) 동작 확인
