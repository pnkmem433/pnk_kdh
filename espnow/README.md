# NFC_I2C_TEST

이 저장소에는 PlatformIO 프로젝트가 2개 있습니다.

- `nfc_master`
  - I2C 마스터
  - 여러 슬레이브의 값을 요청하고, 받은 값을 다시 슬레이브로 에코 전송
- `nfc_slave`
  - RC522에서 값을 읽어 I2C로 보냄
  - 마스터가 에코로 돌려준 값을 화면에 표시

## 동작 흐름

1. `nfc_slave`가 NFC 값을 읽음
2. `nfc_slave`가 값을 I2C 전송 큐에 등록(여러 개 가능)
3. `nfc_master`가 I2C로 값 목록 요청
4. `nfc_slave`가 큐에 있는 값을 **한 번의 요청에서 모두 응답**하고, 응답한 항목은 큐에서 삭제
5. `nfc_master`가 각 값을 에코 전송
6. `nfc_slave`가 에코 값을 화면에 표시

## I2C 다이어그램

```text
[SLAVE] NFC 인식
   |
   |  send({ .value = "AA:BB:CC" })
   |  send({ .value = "HELLO" })
   v
[SLAVE 큐]  ["AA:BB:CC", "HELLO"]
   |
   |  (MASTER TaskRunner에서 scanSlaves 호출)
   v
[SLAVE 응답]  Count=2 + (Len+Data) + (Len+Data)
   |
   v
[MASTER 수신]  콜백 2회 실행
   |
   |  echo("AA:BB:CC")
   |  echo("HELLO")
   v
[SLAVE 에코 수신]  화면 표시
```

## 코드 구조

- 기능별 라이브러리 분리
  - I2C 마스터: `I2CMaster`
  - I2C 슬레이브: `I2CSlave`
  - NFC 리더: `NfcReader` (RC522)
  - 디스플레이: `Display`
  - 로그 출력: `Logger`
  - 태스크 실행: `TaskRunner`
  - HEX 문자열: `HexFormat`
- `main.cpp`
  - 하드웨어/소프트웨어 객체 생성만 담당
  - `setup()/loop()`는 `src/app.cpp`에 위치
  - 모든 설정은 `.field = ...` 지정 방식 사용

## 마스터 설정 위치

`nfc_master/src/main.cpp`

```cpp
TaskRunner masterTask({
  .name = "i2c_scan",
  .stackSize = 4096,
  .priority = 1,
  .core = -1,
});

I2CMaster i2cMaster({
  .slaves = { 0x08, 0x09 },
  .sda = D4,
  .scl = D5,
  .frequency = 100000,
  .task = &masterTask,
});
```

## 마스터 시작

`nfc_master/src/app.cpp`

```cpp
i2cMaster.begin({
  .onDataReceive = ...,
  .intervalMs = 500,
  .startDelayMs = 200,
});
```

## 슬레이브 설정 위치

`nfc_slave/src/main.cpp`

```cpp
TaskRunner nfcTask({
  .name = "nfc_loop",
  .stackSize = 8192,
  .priority = 1,
  .core = -1,
});

TaskRunner echoTask({
  .name = "echo_loop",
  .stackSize = 8192,
  .priority = 1,
  .core = -1,
});

I2CSlave i2cSlave({
  .address = 0x08,
  .sda = 2,
  .scl = 3,
  .frequency = 100000,
  .task = &echoTask,
});

NfcReader nfcReader({
  .pin = { ... },
  .task = &nfcTask,
  .settings = { ... },
});
```

## 슬레이브 시작

`nfc_slave/src/app.cpp`

```cpp
i2cSlave.begin({
  .onReceive = ...,
  .intervalMs = 50,
  .startDelayMs = 0,
});
```

## 로그 출력 형식

`Logger::print()`를 사용하며 `.value/.args` 형태입니다.

```cpp
Logger::print({
  .value = "슬레이브 %s: %s",
  .args = { addrText.c_str(), value.c_str() }
});
```

## 배선

배선 정보는 `배선.md`를 참고하세요.

## 빌드 설정

ESP32 툴체인 이슈로 `-std=gnu++2a`를 사용합니다.

## 사용 라이브러리

- `MFRC522` (RC522 UID 읽기)
- `TFT_eSPI` (T-Display-S3 화면 출력)
- `lib/nfc` (프로젝트 내부 NFC 라이브러리)
- `lib/HexFormat` (0x/0X 형식 문자열 변환)
