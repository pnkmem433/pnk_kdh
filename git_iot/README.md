# FXCO Smart Hanger IoT - 상세 기술 문서 (READMD)

## 0. 문서 목적
이 문서는 프로젝트를 처음 받는 개발자/운영자가 **코드를 빠르게 이해하고, 배포/디버깅/확장까지 바로 수행**할 수 있도록 작성한 상세 기술 문서다.

---

## 1. 프로젝트 한 줄 요약
NFC 태그 감지(Scan/Remove)와 충전 레일 상태를 이용해 행거의 상태 이벤트를 생성하고, 이를 HTTP/MQTT로 백엔드에 전달하며 TFT UI에 실시간 표시하는 ESP32-C3 기반 IoT 펌웨어.

## 1-1. UUID를 사용하는 이유
- 장치 1대마다 **고유 식별자**가 필요합니다.
- 재부팅 후에도 같은 장치로 인식되어야 하므로 UUID를 EEPROM에 저장/재사용합니다.
- HTTP API와 MQTT 토픽에서 \"어떤 행거의 이벤트인지\" 구분하는 기준값입니다.
- 운영 시 로그 추적, 장애 분석, 장치 교체 이력 관리의 기본 키로 사용됩니다.
- OTA 운영 관점에서도 핵심입니다.
  - UUID가 없으면 장치 고유번호를 사람이 일일이 기록/매칭해 업데이트 대상을 수동 관리해야 합니다.
  - UUID를 장치에서 랜덤 생성(최초 1회)하고 유지하면, 같은 펌웨어를 공통 사용하면서도 서버가 장치를 자동 식별해 업데이트 관리를 단순화할 수 있습니다.

## 1-2. EEPROM을 사용하는 이유
- UUID를 전원이 꺼져도 유지하려면 **비휘발성 저장소**가 필요합니다.
- RAM은 재부팅 시 초기화되므로 장치 ID가 사라집니다.
- EEPROM에 UUID를 저장하면 재부팅/전원 차단 후에도 같은 UUID를 다시 읽을 수 있습니다.
- 결과적으로 서버, MQTT, OTA에서 동일 장치로 계속 추적/관리할 수 있습니다.
- SPIFFS/LittleFS 같은 파일시스템도 비휘발성이지만, 파일 쓰기/갱신 중 전원 차단 시 파일시스템 오류 가능성이 있습니다.
- 이 프로젝트는 UUID처럼 매우 작은 고정 데이터만 저장하면 되므로, 파일시스템보다 EEPROM 방식이 단순하고 전원 차단 리스크를 줄이기 유리합니다.

---

## 2. 하드웨어/펌웨어 구성
### 2.1 보드/주변장치
- MCU: Seeed XIAO ESP32C3
- NFC: MFRC522 (SPI)
- 디스플레이: GC9A01 원형 TFT (TFT_eSPI)
- 충전 감지: 디지털 입력(`D6`)

### 2.2 충전 신호 규칙
- `HIGH` = 충전 중
- `LOW` = 비충전

코드 반영 위치:
- `src/main.cpp`의 `charge.listen(...)`

---

## 3. 소프트웨어 아키텍처
### 3.1 최상위 오케스트레이션
- 파일: `src/main.cpp`
- 역할:
  - 전체 초기화 순서 관리
  - MQTT 명령 라우팅
  - NFC 이벤트 fan-out
  - FreeRTOS 태스크/큐 생성
  - UI 렌더링 루프 실행

### 3.2 모듈별 책임
- `lib/wifi`: WiFiMulti 기반 다중 AP 연결/재연결
- `lib/mqtt`: MQTT 연결, subscribe/publish, 수신 콜백
- `lib/http`: 워커 태스크 + 큐 기반 HTTP 비동기 처리
- `lib/Request`: 도메인 API 래핑(`/hanger/connect`, `/pickup`, `/pickdown`)
- `lib/nfc`: RC522 폴링, scan/remove 이벤트 생성
- `lib/Ui`: 화면 상태 렌더링(상품/QR/알림/네트워크)
- `lib/uuid`: EEPROM UUID 유지
- `lib/clothData`: 서버 JSON -> 내부 모델 파싱
- `lib/FirmwareUpdater`: OTA 업데이트 처리

---

## 4. 부팅 시퀀스 상세
`setup()` 실행 흐름:
1. Serial 시작
2. 랜덤 지연값 생성(`delayTime`) - timing takeover 충돌 완화
3. UI init + 로고
4. UUID 로드/생성
5. Wi-Fi 연결
6. 충전 센서 시작
7. OTA 업데이트 확인/적용
8. 서버 connect 요청 (`request.begin(uuid)`)
9. UI에 UUID 전달
10. MQTT 연결
11. command/eventCallback 토픽 구독
12. MQTT 수신 핸들러 등록
13. NFC 콜백 등록
14. 충전 상태 리스너 등록
15. 의류 데이터 없으면 setup QR 안내 루프
16. 의류 데이터 있으면 5초 안내 후 정상 모드
17. 큐 생성 (`appEventQ`, `mqttFastQ`)
18. HTTP 처리 태스크 생성
19. MQTT 고속 전송 태스크 생성
20. NFC 태스크 시작
21. timing 동기화 브로드캐스트 1회 발행
22. 유지보수 태스크 생성(알림 만료 + coordinator/follower 제어)

---

## 5. 이벤트 파이프라인 설계 의도
### 5.1 큐를 분리한 이유
- `appEventQ`: HTTP 호출 담당 (상대적으로 느림)
- `mqttFastQ`: MQTT 즉시 발행 담당 (실시간 요구)

NFC 이벤트를 두 큐로 동시에 보내서, HTTP 지연이 MQTT 실시간성을 방해하지 않게 분리.

### 5.2 NFC 이벤트 정의
- `Scan`: 카드 감지
- `preRemove`: 제거 직전 훅
- `Remove`: 카드 제거 확정

### 5.3 도메인 매핑
- `Scan` -> `pickdown`
- `Remove` -> `pickup`

---

## 6. MQTT 통신 명세
### 6.1 구독 토픽
- `smart_hanger/command/all/#`
- `smart_hanger/command/{uuid}/#`
- `smart_hanger/eventCallback/{uuid}/#`

### 6.2 발행 토픽
- 이벤트: `smart_hanger/event/{uuid}`
- 명령 콜백: `smart_hanger/callback/{uuid}`
- 타이밍 브로드캐스트: `smart_hanger/command/all`

### 6.3 command action
- `notify` : 알림 표시(이름+색상)
- `clear_notify` : 알림 해제
- `restart` : 장치 재부팅
- `timing` : 애니메이션 위상 동기화
- `show_network` : 네트워크 오버레이 표시
- `hide_network` : 오버레이 숨김
- `isGroupCoordinator` : 역할 조회

### 6.4 콜백 폭주 완화
명령 콜백 발행 전 `50~500ms` 랜덤 지연을 적용해, 브로드캐스트 응답 동시 폭주를 완화.

---

## 7. HTTP API 명세
도메인 래퍼: `lib/Request`

- `POST /hanger/connect`
  - body: `{"uuid":"..."}`
  - 역할: 장치 등록 + 의류 데이터 수신

- `POST /hanger/pickdown`
  - body: `{"uuid":"...","pickdownUuid":"..."}`

- `POST /hanger/pickup`
  - body: `{"uuid":"..."}`

- `POST /time-tracking`
  - 이벤트 처리 지연 추적용

---

## 8. UI 상태 머신
렌더링 우선순위 (`loop()`):
1. `notifyStart != 0` -> `showNotify()`
2. 아니면 5초 간격으로
   - `showQr()`
   - `showAllColors()` 교차 표시

추가 상태:
- MQTT 연결 불가 시 배지 표시
- 충전 상태 아이콘 표시
- 네트워크 디버그 오버레이 표시/숨김

---

## 9. 타이밍 동기화(coordinator/follower)
### 9.1 coordinator
- 30분마다 `timing` 전송

### 9.2 follower
- 40분 이상 timing 미수신 시 takeover 시도
- `delayTime(랜덤)`를 더해 동시 takeover 충돌 완화

---

## 10. 설정 파일
### 10.1 PlatformIO
- 파일: `platformio.ini`
- 환경: `seeed_xiao_esp32c3`
- 프레임워크: `arduino`

### 10.2 TFT_eSPI 설정 정책
- 현재 프로젝트 정책: `User_Setup_Select.h`에서 보드/드라이버 설정을 선택해 사용
- 주의: build_flags 강제 설정과 혼용하면 충돌 가능

---

## 11. 배포 전 필수 점검
1. Wi-Fi SSID/PW 실제 환경값 반영
2. HTTP/MQTT 서버 주소/포트 확인
3. OTA 서버 접근 가능 여부 확인
4. 충전 신호 HIGH/LOW 극성 재검증
5. NFC 스캔/제거 반복 테스트
6. MQTT topic 수신/발행 로그 검증

---

## 12. 디버깅 체크포인트
### 증상: TFT 미출력
- 쉴드 결합 불량
- TFT_eSPI setup 선택 오류

### 증상: NFC 감지 불안정
- SPI 공유 간섭
- 전원 품질 문제
- 안테나/배선 상태 문제

### 증상: 이벤트가 서버에 안감
- HTTP 서버 unreachable
- MQTT 인증/토픽 mismatch
- Wi-Fi 연결 불안정

---

## 13. 보안/운영 리스크
- 코드 내 자격증명 하드코딩 존재
- 운영 전 `.env`/빌드 시크릿 분리 권장
- MQTT/HTTP 평문 구간에 대한 망 분리 또는 TLS 적용 고려

---

## 14. 향후 개선 제안
1. 설정값(SSID/서버/계정) 외부화
2. OTA 실패 리트라이/롤백 전략 강화
3. 이벤트 로컬 버퍼링(오프라인 내구성)
4. 명령 인증/서명 도입
5. 테스트 코드(시뮬레이션 + 하드웨어 통합) 추가

---

## 15. 관련 문서
- 설치/배선/컴파일 가이드: `SETUP_GUIDE.md`
- 실행 코드 진입점: `src/main.cpp`
