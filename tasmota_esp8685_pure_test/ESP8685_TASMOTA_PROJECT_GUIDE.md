# ESP8685 Tasmota 작업 정리

## 한눈에 요약

### 목적

- 자체제작 스마트 플러그 하드웨어에 `Tasmota`를 이식한다.
- 대상 칩은 `ESP8685`이며, 계열상 `ESP32-C3` 빌드를 사용한다.
- 목표는 단순 부팅이 아니라 아래 5가지를 모두 확인하는 것이다.
  - 부트 모드 진입
  - 펌웨어 업로드
  - AP 또는 공유기 Wi-Fi 연결
  - Web UI 접속
  - MQTT 송수신

### 현재까지 확인된 것

- `ESP8685` 칩 인식 성공
- `erase-flash` 성공
- `firmware.factory.bin` 업로드 성공
- Tasmota AP 동작 확인
- 공유기 접속 후 Web UI 접속 확인
- MQTT 브로커 연결 확인
- 릴레이 GPIO 확인: `GPIO4`
- 상태 LED GPIO 확인: `GPIO6`
- 버튼 GPIO 확인: `GPIO20`

### 아직 남은 것

- 전력측정 칩 GPIO 최종 식별
- `ENERGY` 값이 Web UI와 MQTT에 실제로 뜨는지 확인
- 버튼 동작을 최종 템플릿에 반영
- OTA 업데이트 절차 최종 검증

### 지금 가능한 것

- 펌웨어 빌드
- `factory` / `OTA` 업로드
- Web UI 접속
- 릴레이 ON/OFF 제어
- LED 상태 확인
- 버튼 GPIO 테스트
- 기본 Tasmota MQTT 토픽 확인
- 커스텀 `smart_plug/...` MQTT 토픽 확인

---

## 1. 이 폴더의 역할

작업 폴더:

- [tasmota_esp8685_pure_test](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test)

이 폴더는 `ESP8685` 기반 스마트 플러그에 맞춰 Tasmota를 테스트하고, 필요한 최소한의 커스텀 기능을 얹는 작업용 폴더다.

핵심 방향은 아래와 같다.

1. Tasmota 본체는 최대한 그대로 유지한다.
2. 우리 하드웨어에 맞는 기본 설정만 덮어쓴다.
3. 필요한 MQTT 브리지 기능만 별도 드라이버로 추가한다.
4. 최종적으로는 OTA 가능한 운영 이미지까지 만든다.

---

## 2. 하드웨어와 펌웨어 관계

### 실제 대상

```text
자체제작 스마트 플러그 보드
        |
        +-- ESP8685 모듈
        +-- 릴레이
        +-- 상태 LED
        +-- 버튼
        +-- 전력측정 칩(미확정)
```

### 펌웨어 구조

```text
Tasmota 기본 코드
        |
        +-- Wi-Fi
        +-- Web UI
        +-- MQTT
        +-- OTA
        +-- GPIO 템플릿/모듈 시스템
        |
        +-- user_config_override.h
        |      -> 기본 Wi-Fi / MQTT / 로그 / 이름 설정
        |
        +-- xdrv_98_smartplug_custom.ino
               -> smart_plug/... 형태의 커스텀 MQTT 브리지
```

---

## 3. 주요 파일 설명

### 설정 파일

- [platformio.ini](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test\platformio.ini)
  - PlatformIO 전체 빌드 설정
- [platformio_tasmota_cenv.ini](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test\platformio_tasmota_cenv.ini)
  - Tasmota 환경별 빌드 정의

### 사용자 덮어쓰기 설정

- [user_config_override.h](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test\tasmota\user_config_override.h)
  - 기본 Wi-Fi
  - 기본 MQTT 브로커 주소
  - 기본 토픽 이름
  - 로그 레벨
  - Friendly Name
  - 커스텀 드라이버 활성화

현재 이 파일에서 중요한 값은 아래와 같다.

```text
Wi-Fi SSID  : CC-Retail
Wi-Fi PASS  : pnks1111
MQTT HOST   : 192.168.0.15
MQTT PORT   : 1883
MQTT USER   : plugtest
MQTT PASS   : fcfc50kc35
MQTT_TOPIC  : tasmota_%06X
```

### 커스텀 MQTT 드라이버

- [xdrv_98_smartplug_custom.ino](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test\tasmota\tasmota_xdrv_driver\xdrv_98_smartplug_custom.ino)

이 파일의 역할:

- 기본 Tasmota MQTT는 그대로 둔다.
- 동시에 아래 커스텀 토픽도 사용 가능하게 한다.

```text
smart_plug/<uid>/status
smart_plug/<uid>/metrics
smart_plug/<uid>/command
```

여기서 `<uid>`는 두 가지 별칭을 함께 지원하도록 작업 중이다.

```text
1. MAC 기반 UID 예: 3C0F021851A4
2. Tasmota topic 예: tasmota_1851A4
```

즉 최종 목표는 둘 다 받는 것이다.

```text
smart_plug/3C0F021851A4/command
smart_plug/tasmota_1851A4/command
```

### 빌드 후 GDrive 복사 스크립트

- [copy-gdrive-firmware.py](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test\pio-tools\copy-gdrive-firmware.py)

이 스크립트의 역할:

- 빌드 성공 후 `firmware.bin`을 지정한 구글드라이브 폴더로 복사
- 파일명 규칙:

```text
v1_esp8685_tasmota.bin
v2_esp8685_tasmota.bin
v3_esp8685_tasmota.bin
...
```

- 현재 구글드라이브에 있는 가장 높은 버전보다 큰 숫자만 허용

---

## 4. 빌드 결과물

주요 산출물:

- [firmware.bin](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test\.pio\build\tasmota32c3-work\firmware.bin)
- [firmware.factory.bin](c:\WS\vs_kdh\pnk_kdh\tasmota_esp8685_pure_test\.pio\build\tasmota32c3-work\firmware.factory.bin)

의미:

- `firmware.factory.bin`
  - 초기 전체 플래시에 사용
  - 주소 `0x0`에 쓰는 이미지
- `firmware.bin`
  - OTA 업그레이드용 일반 이미지

---

## 5. 업로드 모드와 실행 모드 정리

ESP8685는 `GPIO9` 상태에 따라 업로드 모드와 실행 모드가 갈린다.

### 개념 그림

```text
[업로드 모드]
GPIO9 --- GND
        |
        +-- ROM bootloader 진입
        +-- esptool 연결 가능
        +-- erase / write 가능

[실행 모드]
GPIO9 --- 해제
        |
        +-- 플래시된 펌웨어 실행
        +-- Wi-Fi / Web UI / MQTT 동작
```

### 실제 작업 흐름

```text
1. 전원 분리
2. GPIO9를 GND에 연결
3. USB-TTL 3.3V / GND / TX / RX 연결
4. PC 연결
5. esptool로 칩 인식
6. erase-flash
7. firmware.factory.bin 업로드
8. 전원 분리
9. GPIO9-GND 해제
10. 다시 전원 인가
11. 실행 모드 부팅
```

### 중요한 경험적 결론

- 업로드할 때는 `USB-TTL의 3.3V 전원`으로만 잡았을 때 더 안정적이었다.
- 외부전원과 USB-TTL 전원을 동시에 쓰면 업로드가 불안정해질 수 있었다.
- 즉, 업로드 시점에는 전원 경로를 단순하게 가져가는 것이 유리했다.

---

## 6. Web UI와 Wi-Fi 관련 정리

### 초기에 겪은 현상

```text
factory.bin 업로드
-> Tasmota AP 생성
-> 휴대폰/PC 연결
-> 192.168.4.1 접속 시 ERR_EMPTY_RESPONSE
```

이때 확정된 사실:

- AP는 떴다.
- 즉 펌웨어가 아예 죽은 상태는 아니었다.
- 그러나 HTTP 응답 단계는 불안정했다.

이후 다시 정리된 결론:

- 공식 문서 기준으로 "factory.bin 후에 일반 bin을 한 번 더 올려야 Web UI가 열린다"는 규정은 없다.
- 실제 원인은 아래 후보 중 하나였다.

```text
1. 초기 빌드/초기화 불안정
2. 전원 문제
3. 브라우저 또는 접속 경로 문제
4. 플래시 후 설정 상태 문제
```

최종적으로는 공유기 Wi-Fi에 붙은 뒤 `192.168.1.171`에서 Web UI 접속을 확인했다.

---

## 7. MQTT 구조 정리

### 기본 Tasmota MQTT 토픽

기본적으로 보이는 구조:

```text
tele/tasmota_1851A4/LWT
tele/tasmota_1851A4/STATE
tele/tasmota_1851A4/INFO1
tele/tasmota_1851A4/INFO2
tele/tasmota_1851A4/INFO3
tele/tasmota_1851A4/SENSOR
stat/tasmota_1851A4/RESULT
cmnd/tasmota_1851A4/...
```

이 구조는 Tasmota 표준 구조라서 MQTT Explorer에서 그대로 확인 가능하다.

### 커스텀 스마트플러그 토픽

추가로 만들고 있는 구조:

```text
smart_plug/<uid>/status
smart_plug/<uid>/metrics
smart_plug/<uid>/command
```

예시:

```text
smart_plug/3C0F021851A4/status
smart_plug/3C0F021851A4/metrics
smart_plug/3C0F021851A4/command

smart_plug/tasmota_1851A4/status
smart_plug/tasmota_1851A4/metrics
smart_plug/tasmota_1851A4/command
```

### 주의할 점

아래처럼 보내면 안 된다.

```text
smart_plug/tasmota_1851A4\command
```

이건 `\` 백슬래시가 들어가 있으므로, 실제 구독 토픽과 다른 문자열이다.

올바른 형태는 반드시 슬래시 `/` 이다.

```text
smart_plug/tasmota_1851A4/command
```

### 기대하는 명령 페이로드

```json
{"cmd":"ON"}
```

또는

```json
{"cmd":"OFF"}
```

또는 plain text:

```text
ON
OFF
TOGGLE
STATUS
```

---

## 8. 지금까지 확인된 GPIO 결과

현재 실사용 결과:

```text
GPIO4  = Relay1
GPIO6  = Led1 또는 Led_i1 후보
GPIO20 = Button1
GPIO21 = TX
```

### 상태 그림

```text
GPIO4  ----> 릴레이 제어 확인 완료
GPIO6  ----> LED 점등 확인 완료
GPIO20 ----> 버튼 입력 확인 완료
GPIO21 ----> UART TX
```

### 버튼/릴레이/LED 관계

Tasmota 기본 구조는 아래와 같다.

```text
Button1 입력
    -> Power Toggle
    -> Relay1 상태 변경
    -> LedState 설정에 따라 LED 상태 반영
```

즉 버튼 GPIO만 맞으면, 별도 로직을 새로 짜지 않아도 기본 Tasmota 동작만으로도 충분히 제어가 이어진다.

---

## 9. 템플릿과 실제 반영 시 주의점

중요한 점:

- `Template` JSON이 잘못 입력되면 적용되지 않을 수 있다.
- Web UI에서 GPIO를 바꿔도 저장/재시작 흐름이 어긋나면 반영이 안 된 것처럼 보일 수 있다.
- 실제로는 `Module 0` 상태에서 각 GPIO를 수동 지정해가며 검증하는 방식이 가장 확실했다.

예:

```text
GPIO4  -> Relay1
GPIO6  -> Led1
GPIO20 -> Button1
```

---

## 10. 지금까지의 진행 흐름

### 전체 흐름 요약

```text
[1] 첫 번째 스마트 플러그
    -> 납땜 패드 손상
    -> 교체

[2] 새 스마트 플러그 / 새 ESP8685
    -> USB-TTL 연결
    -> 업로드 모드 확인
    -> erase-flash
    -> factory 업로드
    -> AP 확인
    -> Web UI / MQTT 확인

[3] GPIO 탐색
    -> Relay GPIO 확인
    -> LED GPIO 확인
    -> Button GPIO 확인

[4] MQTT 브리지 추가
    -> 기본 Tasmota 토픽 유지
    -> smart_plug/... 토픽 추가

[5] 남은 일
    -> 전력측정 칩 GPIO 확정
    -> ENERGY 값 활성화
```

### 시간 순서 개념도

```text
칩 인식
  -> 플래시 삭제
  -> factory 업로드
  -> 실행 모드 부팅
  -> AP 또는 공유기 연결
  -> Web UI 확인
  -> MQTT 확인
  -> 릴레이/LED 확인
  -> 버튼 확인
  -> 에너지 측정 확인 예정
```

---

## 11. 지금 빌드가 하는 일

현재 커스텀 빌드는 아래를 동시에 하도록 설계되어 있다.

```text
1. Tasmota 기본 기능 제공
2. 기본 MQTT 브로커 자동 설정
3. Tasmota 표준 토픽 유지
4. smart_plug/... 보조 토픽 추가
5. OTA용 firmware.bin 생성
6. factory용 firmware.factory.bin 생성
7. 빌드 후 구글드라이브 버전 파일 복사
```

---

## 12. 앞으로 바로 해야 할 일

### 우선순위 1

- 새 빌드 업로드 후 MQTT Explorer에서 아래 두 토픽이 모두 보이는지 확인

```text
smart_plug/3C0F021851A4/status
smart_plug/tasmota_1851A4/status
```

- 아래 두 명령 중 하나로 상태 변경 확인

```text
topic   : smart_plug/3C0F021851A4/command
payload : {"cmd":"ON"}
```

```text
topic   : smart_plug/tasmota_1851A4/command
payload : {"cmd":"OFF"}
```

### 우선순위 2

- 버튼 GPIO20을 최종 템플릿에 고정
- 부팅 후에도 유지되는지 확인

### 우선순위 3

- 전력측정 칩 GPIO 후보를 실제 보드와 대조
- `HLW8012`, `BL0937`, `CSE7766`, `BL0942` 계열 중 어느 쪽인지 확정
- `tele/.../SENSOR`에 `ENERGY`가 뜨는지 확인

---

## 13. 현재 결론

이 프로젝트는 이미 단순 실험 단계를 넘어서, 실제 동작 가능한 ESP8685용 Tasmota 이식 단계까지 왔다.

현재 확정된 핵심은 아래와 같다.

- ESP8685는 `ESP32-C3` 계열 빌드로 접근 가능하다.
- 업로드 모드와 실행 모드는 `GPIO9` 상태로 구분된다.
- Web UI와 MQTT는 이미 동작 가능한 상태까지 확인했다.
- 하드웨어 제어의 기본 3요소인 릴레이, LED, 버튼 GPIO는 거의 정리됐다.
- 남은 핵심은 전력측정 GPIO 확정과 `ENERGY` 활성화다.

즉, 지금부터는 "아예 안 되는 상태"가 아니라, "기본 동작은 확보했고 세부 기능을 맞추는 단계"라고 보는 것이 맞다.
