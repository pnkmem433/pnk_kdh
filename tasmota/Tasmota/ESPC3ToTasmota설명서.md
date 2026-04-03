# ESPC3 To Tasmota 설명서

이 문서는 **ESP32-C3/ESP 계열용 Arduino 스타일 커스텀 펌웨어**를 **공식 Tasmota 소스코드 기반 커스텀 빌드**로 옮길 때, AI가 그대로 따라 작업할 수 있도록 만든 변환 지침서다.

목표는 기존 `main.cpp` 전체를 Tasmota 안으로 억지로 넣는 것이 아니라, 다음처럼 역할을 분리하는 것이다.

- Tasmota가 맡는 것: Wi-Fi, MQTT 세션, OTA, WebUI, 설정 저장, 템플릿, 기본 버튼/릴레이 프레임워크
- 사용자 코드가 맡는 것: 버튼 눌렀을 때 동작, MQTT 명령 해석, 상태 payload 생성, 주기 상태 전송

이 문서는 다음 저장소 구조를 기준으로 한다.

- 작업 루트: `c:\WS\vs_kdh\pnk_kdh\tasmota`
- 공식 Tasmota 소스: `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota`

---

## 0. 공식 문서에서 반드시 지켜야 하는 원칙

이 항목은 Tasmota 공식 `Firmware Builds` / `Compiling` 문서에서 가져온 핵심 규칙이다.

### 0.1 ESP8266 계열은 펌웨어 크기 여유가 매우 중요함

- ESP82XX 계열은 플래시 메모리가 제한적이다.
- OTA를 안정적으로 하려면 펌웨어를 **가능하면 625KB 미만**으로 유지하는 것이 좋다.
- 기능을 많이 넣으면 컴파일은 되더라도 OTA 업로드 여유가 부족할 수 있다.

AI는 빌드 후 결과 크기를 반드시 확인하고, 너무 커지면 불필요한 기능을 줄이는 방향을 먼저 고려한다.

### 0.2 기능 추가/제거는 `tasmota` 또는 `tasmota32` 기준으로만 할 것

중요:

- `tasmota-sensors`, `tasmota-display`, `tasmota-lite` 같은 **변형 펌웨어에 기능을 추가하거나 제거하지 않는다.**
- 공식 문서 기준으로 기능 추가/제거는 **기본 `tasmota` 또는 `tasmota32` 환경에서만** 지원되는 방식으로 본다.

따라서 AI는:

- ESP8266이면 `env:tasmota`를 기반으로 커스텀 env를 만든다.
- ESP32면 `env:tasmota32_base` 또는 `env:tasmota32` 계열을 기반으로 커스텀 env를 만든다.

### 0.3 `tasmota-minimal.bin`은 초기 설치용이 아님

- `tasmota-minimal.bin`은 OTA 업로드를 돕기 위한 특수 빌드다.
- 초기 설치용으로 쓰지 않는다.
- 특별히 두 단계 OTA 전략이 필요한 경우가 아니면 기본 `tasmota` 기반 커스텀 빌드를 우선 사용한다.

### 0.4 기본적으로는 사전 컴파일 바이너리를 우선 검토

- 요구사항이 공식 빌드로 충족되면 직접 컴파일할 필요가 없다.
- 하지만 이 문서의 대상은 **사용자 로직을 직접 넣어야 하는 경우**이므로, 커스텀 빌드를 전제로 한다.

### 0.5 `my_user_config.h`는 수정하지 말 것

- 사용자 기본 설정 변경은 `user_config_override.h`
- 빌드 환경 분기는 `platformio_tasmota_cenv.ini`
- 기본 빌드 env 선택은 `platformio_override.ini`

로 처리한다.

### 0.6 `USER_BACKLOG`와 `USER_RULE`를 적극 고려할 것

Tasmota 공식 문서상 아래 항목은 커스텀 코드를 줄이는 데 유용하다.

- `USER_BACKLOG`
- `USER_RULE1`, `USER_RULE2`, `USER_RULE3`

즉 AI는 C++ 코드로 옮기기 전에 먼저 판단해야 한다.

1. 이 동작이 단순 설정 자동화인가?
2. 이 동작이 규칙 엔진으로 가능한가?
3. 굳이 `xdrv` 코드까지 내려갈 필요가 있는가?

예:

- 초기 MQTT 설정
- 기본 옵션 설정
- 초기화 후 특정 명령 일괄 실행
- `.dmp` 자동 복구

이런 것은 `USER_BACKLOG`로 해결 가능하다.

예시:

```c
#define USER_BACKLOG "WebGetConfig http://myserver/tasmota/conf/%id%.dmp"
```

따라서 AI는 항상 먼저 아래 순서로 판단한다.

1. `user_config_override.h`만으로 해결 가능한가
2. `USER_BACKLOG` / `USER_RULE`로 해결 가능한가
3. 그래도 안 되면 `xdrv_98_*.ino`로 간다

---

## 1. 최종 출력물

AI는 반드시 아래 파일들만 생성하거나 수정한다.

1. `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\platformio_override.ini`
2. `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\platformio_tasmota_cenv.ini`
3. `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\tasmota\user_config_override.h`
4. `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\tasmota\tasmota_xdrv_driver\xdrv_98_<custom_name>.ino`

필요 시 아래 파일도 수정한다.

5. `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\.gitignore`

주의:

- `my_user_config.h`는 수정하지 않는다.
- 공식 Tasmota 코어 파일을 가능하면 직접 수정하지 않는다.
- 새 기능은 `xdrv_98_*` 또는 필요하면 `xsns_*` 새 파일로 넣는다.

---

## 2. 입력 코드에서 분류할 것

ESP-C3 원본 코드에서 다음 항목을 먼저 추출한다.

### 2.0 사용자 입력 없이 먼저 자동 추출할 것

AI는 아래 값들을 **반드시 원본 ESP-C3 코드에서 먼저 찾는다.**
값이 코드에 있으면 사용자에게 다시 묻지 않는다.

- Wi-Fi SSID
- Wi-Fi Password
- MQTT Host
- MQTT Port
- MQTT Username
- MQTT Password
- OTA 서버 URL
- 프로젝트 ID
- 현재 버전 코드
- 버튼 GPIO
- 릴레이 GPIO
- LED GPIO
- 상태 토픽 규칙
- 명령 토픽 규칙
- 상태 payload 형식

값을 찾는 우선순위는 아래와 같다.

1. `const char*`, `constexpr`, `#define` 상수
2. 객체 생성자 인자
3. `platformio.ini` 또는 빌드 플래그
4. 하드코딩 문자열
5. 주석

예:

- `WifiManager wifi = WifiManager({.ssid = "CC-Retail", .password = "pnks1111"});`
- `mqtt.begin("gym907-0001.iptime.org", "pnks", "pnks1111");`
- `Sensor button = Sensor(20);`
- `DigitalOut relay = DigitalOut(4);`

이런 코드는 사용자에게 묻지 말고 자동으로 추출한다.

### 2.1 Tasmota가 대신하는 코드

아래는 Tasmota로 갈 때 보통 버리거나 직접 옮기지 않는다.

- `WifiManager wifi(...)`
- `wifi.begin()`
- `wifi.tick()`
- `FirmwareUpdater updater(...)`
- `updater.performFirmwareUpdate(...)`
- `mqtt.begin(...)`
- `mqtt.tick()`
- `uuid.begin()`
- `dateTime.begin()`
- `setup()`에서 하는 Wi-Fi/MQTT/OTA 초기화
- `loop()`에서 하는 네트워크 유지 코드

이 항목들은 Tasmota 기능으로 대체한다.

### 2.2 그대로 개념을 유지할 코드

아래는 Tasmota 안으로 옮길 핵심 로직이다.

- `statusPayload(bool isOn)`
- 버튼 눌렀을 때 릴레이 토글
- `cmd == "on"`, `cmd == "off"`, `cmd == "status"` 처리
- 상태 publish
- 5초 주기 상태 publish

### 2.3 값으로만 옮길 것

다음은 `user_config_override.h` 또는 Tasmota 설정으로 옮긴다.

- Wi-Fi SSID / Password
- MQTT host / port / username / password
- MQTT topic 규칙
- 기본 장치명 규칙

### 2.4 자동 추출 실패 시에만 물어볼 것

아래 값은 **원본 코드에서 찾을 수 없을 때만** 사용자에게 질문한다.

- Wi-Fi SSID / Password
- MQTT host / port / username / password
- 장치별 GPIO 매핑
- MQTT topic naming 규칙
- 장치명 규칙
- Template JSON

질문 원칙:

- 한 번에 많은 질문을 하지 않는다.
- 정말 빌드나 동작에 필요한 값만 묻는다.
- 코드에서 추론 가능한 값은 질문하지 않는다.

좋은 질문 예시:

> 원본 코드에서 MQTT host는 찾았는데 포트가 보이지 않습니다. 기본 `1883`으로 진행해도 될까요?

나쁜 질문 예시:

> Wi-Fi, MQTT, 핀, 토픽 규칙 전부 알려주세요  .

---

## 3. 변환 규칙

### 3.1 `main.cpp`를 그대로 넣지 말 것

Tasmota는 일반 Arduino 스케치처럼 `setup()` / `loop()`를 사용자가 직접 운영하는 구조가 아니다.

대신:

- 초기화: `FUNC_INIT`
- MQTT 구독 등록: `FUNC_MQTT_SUBSCRIBE`
- MQTT 메시지 수신: `FUNC_MQTT_DATA`
- 릴레이 상태 변경 감지: `FUNC_SET_DEVICE_POWER`
- 주기 처리: `FUNC_EVERY_SECOND`
- 필요 시 버튼 이벤트: `FUNC_BUTTON_PRESSED`

에 맞춰 새 `xdrv_98_*.ino` 파일을 만든다.

### 3.1.1 먼저 코드가 정말 필요한지 판단할 것

원래 ESP-C3 코드의 동작이 다음 범주라면 C++ 코드 대신 Tasmota 설정으로 우선 해결한다.

- 기본 Wi-Fi / MQTT 설정
- SetOption 기본값
- 초기 실행 시 명령 자동 실행
- 설정 복구
- 간단한 publish 규칙

이 경우 우선 고려 순서는:

1. `user_config_override.h`
2. `USER_BACKLOG`
3. `USER_RULE`
4. `xdrv_98_*.ino`

### 3.2 Wi-Fi / MQTT 기본값은 `user_config_override.h`

다음처럼 넣는다.

```c
#undef STA_SSID1
#define STA_SSID1 "CC-Retail"

#undef STA_PASS1
#define STA_PASS1 "pnks1111"

#undef MQTT_HOST
#define MQTT_HOST "gym907-0001.iptime.org"

#undef MQTT_PORT
#define MQTT_PORT 1883

#undef MQTT_USER
#define MQTT_USER "pnks"

#undef MQTT_PASS
#define MQTT_PASS "pnks1111"
```

### 3.3 새 빌드 환경은 `platformio_tasmota_cenv.ini`

새 환경 이름 예시:

```ini
[env:tasmota-smartplug]
extends                 = env:tasmota
build_flags             = ${env:tasmota.build_flags}
                          -DUSE_SMARTPLUG_CUSTOM
                          -DCODE_IMAGE_STR='"tasmota-smartplug"'
                          -DOTA_URL='""'
```

중요:

- `extends = env:tasmota` 처럼 **기본 `tasmota`를 기반으로** 만든다.
- `tasmota-sensors`, `tasmota-display`, `tasmota-lite`를 기반으로 새 기능을 얹지 않는다.

### 3.4 기본 빌드 env 선택은 `platformio_override.ini`

예시:

```ini
[platformio]
default_envs = tasmota-smartplug
```

---

## 4. `xdrv_98_*.ino` 작성 규칙

### 4.1 기본 구조

```cpp
#ifdef USE_SMARTPLUG_CUSTOM

#define XDRV_98 98

namespace Spc98 {

// 내부 상태 변수

// 토픽 생성 함수

// payload 생성 함수

// MQTT 명령 처리 함수

// 상태 publish 함수

// 1초 주기 함수

}  // namespace Spc98

bool Xdrv98(uint32_t function) {
  switch (function) {
    case FUNC_INIT:
      ...
      break;
    case FUNC_MQTT_SUBSCRIBE:
      ...
      break;
    case FUNC_MQTT_DATA:
      return ...;
    case FUNC_SET_DEVICE_POWER:
      ...
      break;
    case FUNC_EVERY_SECOND:
      ...
      break;
  }
  return false;
}

#endif
```

### 4.2 함수 이름 규칙

중요:

- Arduino/Tasmota `.ino` 빌드는 자동 프로토타입을 생성한다.
- 전역 함수 이름 충돌이 쉽게 난다.
- 반드시 `namespace`를 사용하고, 함수 호출도 `Spc98::함수명()`처럼 명시한다.

### 4.3 MQTT 토픽 규칙

원래 코드가 `smart_plug/{uuid}/command`, `smart_plug/{uuid}/status`였다면 Tasmota에서는 UUID 대신 기본적으로 `TasmotaGlobal.mqtt_topic`을 사용한다.

예:

- command topic: `smart_plug/<mqtt_topic>/command`
- status topic: `smart_plug/<mqtt_topic>/status`

토픽 생성 예시:

```cpp
snprintf_P(buffer, size, PSTR("smart_plug/%s/%s"), TasmotaGlobal.mqtt_topic, suffix);
```

### 4.4 상태 payload 규칙

원래 코드가 아래였다면:

```cpp
{"state":"on"}
{"state":"off"}
```

Tasmota에서도 동일하게 유지한다.

예:

```cpp
snprintf_P(buffer, size, PSTR("{\"state\":\"%s\"}"), GetStateText(bitRead(TasmotaGlobal.power, 0)));
```

주의:

- `GetStateText()`는 기본적으로 `OFF` / `ON` 대문자를 줄 수 있다.
- 원래 시스템이 소문자 `on` / `off`만 받는다면 소문자로 강제 변환해야 한다.
- 현재 저장소 예시는 단순 상태 전달 기준이므로 `GetStateText()`를 사용했다.

### 4.5 릴레이 제어 규칙

Tasmota에서 직접 GPIO를 `digitalWrite` 하지 말고, 아래 함수를 사용한다.

```cpp
ExecuteCommandPower(1, POWER_ON, SRC_MQTT);
ExecuteCommandPower(1, POWER_OFF, SRC_MQTT);
ExecuteCommandPower(1, POWER_TOGGLE, SRC_BUTTON);
```

직접 GPIO를 건드리면 Tasmota 내부 상태와 어긋날 수 있다.

### 4.6 MQTT 수신 처리 규칙

`FUNC_MQTT_DATA`에서:

1. 현재 수신 topic이 내가 원하는 topic인지 비교
2. JSON parse
3. `cmd` 필드 읽기
4. `on/off/status` 처리

예시:

```cpp
JsonParser parser((char*)XdrvMailbox.data);
JsonParserObject root = parser.getRootObject();
const char* cmd = root.getStr(PSTR("cmd"), "");
```

### 4.7 주기 상태 발행 규칙

원래 `loop()`에서 5초마다 publish 하던 것은 `FUNC_EVERY_SECOND`로 옮긴다.

예:

- 카운터 변수 5로 초기화
- 매초 감소
- 0이 되면 publish 후 다시 5로 초기화

---

## 5. 현재 저장소에서 이미 성공한 예시

다음 파일은 실제 빌드 성공한 예시다. AI는 새 작업 시 이 파일을 우선 참고한다.

- `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\tasmota\tasmota_xdrv_driver\xdrv_98_smartplug_custom.ino`
- `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\tasmota\user_config_override.h`
- `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\platformio_tasmota_cenv.ini`
- `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\platformio_override.ini`

이 예시는 다음 기능을 구현했다.

- `smart_plug/<mqtt_topic>/command` 구독
- `{"cmd":"on"}`, `{"cmd":"off"}`, `{"cmd":"status"}` 처리
- 릴레이 상태를 `smart_plug/<mqtt_topic>/status`로 발행
- 5초마다 상태 발행

추가로, 이 예시는 **기본 `tasmota` env를 확장한 커스텀 env**라는 점도 중요하다.

---

## 5.1 빌드 전 판단 체크리스트

AI는 실제 수정 전에 아래를 먼저 판단한다.

1. 이 작업이 사전 컴파일된 공식 빌드로 해결 가능한가
2. 설정만 바꾸면 되는가
3. `USER_BACKLOG`로 해결 가능한가
4. `USER_RULE`로 해결 가능한가
5. 그래도 안 되면 `xdrv_98_*.ino`가 필요한가

정말 마지막 단계에서만 새 드라이버 코드를 추가한다.

---

## 6. AI가 따라야 하는 작업 순서

다음 순서를 반드시 따른다.

1. 입력 `main.cpp`와 관련 헤더/설정 파일에서 값과 로직을 자동 추출한다.
2. Wi-Fi/MQTT/OTA/UUID 초기화 코드 중 Tasmota가 대신하는 코드를 제거 대상으로 분류한다.
3. 버튼 동작, MQTT 명령 처리, 상태 payload, 주기 publish 로직만 추출한다.
4. 자동 추출 결과를 기준으로 `user_config_override.h`에 Wi-Fi/MQTT 기본값을 넣는다.
5. `platformio_tasmota_cenv.ini`에 새 env를 추가한다.
6. `platformio_override.ini`에서 새 env를 기본 env로 선택한다.
7. `xdrv_98_<name>.ino` 파일에 사용자 로직을 옮긴다.
8. 함수는 반드시 namespace로 감싼다.
9. 릴레이 제어는 `ExecuteCommandPower()`를 사용한다.
10. MQTT publish는 `MqttPublishPayload()`를 사용한다.
11. 자동 추출 실패 항목이 있으면 그때만 사용자에게 짧게 질문한다.
12. 빌드 명령으로 실제 컴파일 확인한다.

---

## 6.1 자동 추출 규칙

AI는 변환 전에 아래 패턴을 먼저 탐색한다.

### Wi-Fi

다음 패턴을 우선 찾는다.

```cpp
WifiManager wifi = WifiManager({.ssid = "...", .password = "..."});
WifiManager wifi("...", "...");
```

찾으면:

- `STA_SSID1`
- `STA_PASS1`

으로 변환한다.

### MQTT

다음 패턴을 우선 찾는다.

```cpp
mqtt.begin("host", "user", "password");
mqtt.begin("host", port, "user", "password");
```

찾으면:

- `MQTT_HOST`
- `MQTT_PORT`
- `MQTT_USER`
- `MQTT_PASS`

으로 변환한다.

포트가 없으면 기본값 `1883`을 사용한다.

### GPIO

다음 패턴을 우선 찾는다.

```cpp
Sensor button = Sensor(20);
DigitalOut relay = DigitalOut(4);
DigitalOut led1 = DigitalOut(6, false);
DigitalOut led2 = DigitalOut(1, false);
```

찾으면 메모해두고, Tasmota Template 또는 장치 설정 시 참고값으로 기록한다.

### OTA

다음 패턴을 찾는다.

```cpp
FirmwareUpdater updater("url", "projectId", version);
```

찾으면:

- Tasmota 기본 OTA로 옮기지 않는다
- 대신 “기존 커스텀 OTA 정보”로 기록만 해둔다
- 필요 시 문서나 주석에 남긴다

### Topic / Payload

다음 패턴을 찾는다.

```cpp
String controlTopic = "smart_plug/{uuid}";
return "{\"state\":\"" + String(isOn ? "on" : "off") + "\"}";
```

찾으면:

- `smart_plug/<mqtt_topic>/command`
- `smart_plug/<mqtt_topic>/status`
- `{"state":"on/off"}`

형태로 우선 변환한다.

---

## 7. 빌드 명령

작업 폴더:

`c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota`

명령:

```powershell
$env:PLATFORMIO_CORE_DIR='c:\WS\vs_kdh\pnk_kdh\tasmota\.platformio-core'
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
```

빌드 성공 시 결과물 예시:

`c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\.pio\build\tasmota-smartplug\firmware.bin`

---

## 8. AI에게 직접 줄 수 있는 작업 지시문

아래 문장을 그대로 AI에게 전달해도 된다.

> `c:\WS\vs_kdh\pnk_kdh\tasmota\Tasmota\ESPC3ToTasmota설명서.md`를 기준으로, 내 ESP-C3용 Arduino 스타일 `main.cpp`를 공식 Tasmota 소스 구조에 맞게 이식해줘.  
> 원본 코드에서 Wi-Fi, MQTT, GPIO, 토픽, payload 형식은 먼저 자동으로 찾아서 반영하고, 정말 찾을 수 없는 값만 마지막에 짧게 물어봐.  
> Tasmota가 맡는 Wi-Fi/MQTT/OTA 초기화 코드는 제거하고, 사용자 로직만 `user_config_override.h`, `platformio_tasmota_cenv.ini`, `platformio_override.ini`, `xdrv_98_<name>.ino`에 반영해줘.  
> 반드시 실제 빌드까지 확인하고, 수정한 파일 경로와 수정 위치를 함께 알려줘.

---

## 9. 한계와 주의사항

- 이 문서는 **로직 이식 자동화**용이다.
- 실제 GPIO 번호와 Template은 장치별로 별도 확인이 필요하다.
- ESP32 전용 라이브러리나 API는 그대로 옮기면 안 된다.
- OTA 서버 프로토콜은 Tasmota 기본 OTA와 별도일 수 있으므로, 기존 커스텀 `FirmwareUpdater`는 그대로 옮기지 않는다.
- 장치별 LED, 버튼, 릴레이 핀 동작은 Tasmota Template 또는 Module 설정으로 해결하는 것이 우선이다.
- Wi-Fi/MQTT/OTA 상태 LED를 쓰려면 Template에서 상태 LED를 반드시 `LEDLNK` 또는 `LEDLNK_INV`로 잡아야 한다.
- 부팅 시그니처처럼 Tasmota 기본 동작에 없는 LED 패턴만 `xdrv_98_*.ino`에서 최소한으로 추가한다.
- Wi-Fi/MQTT 연결 문제 점멸은 Tasmota 기본 LED 로직을 우선 사용하고, OTA는 Tasmota 기본 업그레이드 상태를 재사용한다.

## 12. LED 상태 표시 포팅 규칙

원래 ESP-C3 코드가 다음처럼 단계별 LED를 직접 깜빡였다면:

- 부팅 직후 빠른 점멸
- Wi-Fi 연결 중 500ms 점멸
- OTA 확인 중 250ms 점멸
- MQTT 미연결 시 100ms 점멸

Tasmota로 옮길 때는 다음 순서를 따른다.

1. 먼저 Template에서 상태 LED를 `LEDLNK` 또는 `LEDLNK_INV`로 설정한다.
2. Wi-Fi/MQTT 상태 표시는 Tasmota 기본 `SetLedLink`/`SetOption31` 동작을 그대로 사용한다.
3. OTA 상태 표시는 가능하면 Tasmota 기본 OTA 상태를 그대로 사용한다.
4. 꼭 필요한 부팅 시그니처나 추가 연출만 `xdrv_98_*.ino`의 `FUNC_EVERY_50_MSECOND` 등으로 최소 구현한다.

중요:

- 상태 LED 핀이 Template에 없으면 커스텀 LED 코드를 넣어도 아무것도 보이지 않는다.
- 원래 코드의 `DigitalOut led2 = DigitalOut(1, false);` 같은 숫자를 ESP8266 Tasmota Template에 그대로 옮기면 안 된다.
- ESP-C3 GPIO 번호와 ESP8266/ESP8285 Template GPIO는 직접 대응되지 않을 수 있으므로 실제 보드 기준으로 다시 확인해야 한다.
- 펌웨어 크기가 커지면 ESP8266 OTA가 막힐 수 있으므로, 빌드 후 바이너리 크기를 반드시 확인한다.
- 기본 변형 펌웨어를 커스텀 출발점으로 잘못 선택하면 기능 충돌이나 빌드 실패가 날 수 있다.

---

## 10. 이 문서의 핵심 요약

- `main.cpp` 통짜 이식 금지
- 원본 코드에서 설정값은 먼저 자동 추출
- 공식 빌드/설정/백로그/룰로 해결 가능한지 먼저 판단
- 설정값은 `user_config_override.h`
- 빌드 env는 `platformio_tasmota_cenv.ini`
- 기본 env 선택은 `platformio_override.ini`
- 기능 추가는 기본 `tasmota` 또는 `tasmota32` 기반에서만 진행
- 사용자 로직은 `xdrv_98_*.ino`
- Wi-Fi/MQTT/OTA는 Tasmota가 관리
- 버튼/릴레이/명령/상태 전송만 사용자 코드로 옮긴다
---

## 고정 규칙

- AI는 이 설명서의 규칙 자체를 임의로 추가, 삭제, 변형하지 않는다.
- AI는 프로젝트별 실험 결과, 장치별 GPIO 추정, 실패 이력, 임시 판단을 이 설명서에 적지 않는다.
- 그런 내용은 별도의 작업 노트 파일에만 기록한다.
- AI는 먼저 이 설명서를 고정 기준으로 따르고, 필요한 경우에만 별도 노트를 참고한다.
- 설명서를 바꿔야 하는 경우는 사용자가 명시적으로 “설명서 규칙을 수정하라”고 요청한 경우만 허용한다.

---

## OTA 업그레이드 실전 규칙

이 섹션은 Tasmota 공식 `Upgrading` 문서를 기준으로 한 고정 규칙이다.  
AI는 Tasmota 기반 펌웨어를 다룰 때 아래 규칙을 우선 적용해야 한다.

### 1. Wi-Fi 와 OTA 는 Tasmota 기본 기능을 그대로 사용한다

- 커스텀 포팅 시 별도의 `WifiManager`, 별도의 `FirmwareUpdater`, 별도의 HTTP POST OTA 프로토콜을 새로 넣지 않는다.
- Tasmota에 이미 있는 기능을 우선 사용한다.
- 즉:
  - Wi-Fi 연결/저장/복구 = Tasmota 기본
  - 웹 UI 펌웨어 업그레이드 = Tasmota 기본
  - `OtaUrl` 기반 업그레이드 = Tasmota 기본
- 사용자 코드가 추가하는 것은 원칙적으로 장치 로직, MQTT 명령 처리, 상태 발행, LED 연출 같은 부분만 허용한다.

### 2. OTA URL 의 의미

- Tasmota의 `OtaUrl` 은 API 주소가 아니라 **직접 다운로드 가능한 바이너리 파일 URL** 이어야 한다.
- 예:
  - `http://192.168.0.10:8000/firmware.bin`
  - `http://192.168.0.10:8000/firmware.bin.gz`
- 아래와 같은 전용 서버 프로토콜은 Tasmota 기본 OTA 와 호환되지 않는 것으로 본다.
  - `POST /firmwareDownload`
  - `ProjectId`
  - `currentVersion`
  - manifest JSON 비교 후 스트림 반환
- 위와 같은 구조는 ESP-C3 커스텀 앱 방식일 수 있으므로, Tasmota 기본 OTA 로직과 분리해서 판단한다.

### 3. `.bin.gz` 사용 규칙

- Tasmota 8.2 이상에서는 `.bin.gz` 사용 가능성을 우선 검토한다.
- 가능하면 `.bin` 보다 `.bin.gz` 를 우선 사용한다.
- 이유:
  - 파일 크기가 더 작다
  - OTA 성공 가능성이 높아진다
  - 중간 minimal 단계가 불필요해질 수 있다
- 단, 현재 장치가 Tasmota 8.2 이전이면 `.bin.gz` 는 사용하지 않는다.

### 4. `tasmota-minimal.bin(.gz)` 가 필요한 조건

- 업그레이드할 바이너리가 큰 경우, 새 바이너리를 저장할 플래시 여유 공간이 부족할 수 있다.
- Tasmota 공식 업그레이드 문서 기준으로 파일 업로드 대상이 **500KB 보다 크면** `tasmota-minimal.bin(.gz)` 선행 업그레이드가 필요할 수 있다고 본다.
- 따라서 AI는 빌드 후 파일 크기를 반드시 확인하고 다음처럼 판단한다.

#### 4.1 바로 업그레이드 가능 후보

- `.bin.gz` 로 충분히 작다
- 또는 `.bin` 이 500KB 이하
- 이 경우:
  - 웹 UI 파일 업로드로 바로 시도 가능
  - 또는 `OtaUrl` 에 직접 `.bin` / `.bin.gz` 주소를 넣어 시도 가능

#### 4.2 minimal 선행 필요 후보

- `.bin` 이 500KB 초과
- OTA 또는 파일 업로드 시 버퍼 부족 / 업로드 버퍼 불일치 / 공간 부족 가능성이 높음
- 이 경우:
  1. `tasmota-minimal.bin` 또는 `tasmota-minimal.bin.gz` 먼저 업그레이드
  2. 재부팅 후 웹 UI 재접속
  3. 최종 커스텀 `firmware.bin` 또는 `firmware.bin.gz` 업그레이드

### 5. 웹 UI 업그레이드 절차

AI는 웹 UI 방식일 때 다음 순서를 기본으로 안내한다.

#### 5.1 파일 업로드 방식

1. 현재 설정을 백업한다.
2. 업로드할 파일 크기를 확인한다.
3. 파일이 크면 `tasmota-minimal.bin(.gz)` 를 먼저 올린다.
4. 기기가 재부팅될 때까지 기다린다.
5. 다시 웹 UI 로 접속한다.
6. 최종 커스텀 `firmware.bin` 또는 `firmware.bin.gz` 를 올린다.
7. 재부팅 후 버전, Wi-Fi, MQTT, LED 동작을 확인한다.

#### 5.2 URL OTA 방식

1. 먼저 로컬 또는 사설 HTTP 서버에 최종 바이너리를 올린다.
2. URL 은 반드시 직접 파일 경로여야 한다.
3. 파일이 크면 minimal 을 먼저 적용한다.
4. 그 다음 `OtaUrl http://server/yourbinary.bin(.gz)` 형태로 설정한다.
5. `Upgrade 1` 또는 웹 UI 업그레이드 시작으로 진행한다.

### 6. 사설 HTTP OTA 서버 규칙

- 테스트용 HTTP 서버는 간단한 정적 파일 서버면 충분하다.
- 예:

```powershell
python -m http.server 8000
```

- 그 다음 `OtaUrl` 은 예를 들어 아래처럼 사용한다.

```text
http://192.168.0.15:8000/firmware.bin
http://192.168.0.15:8000/firmware.bin.gz
```

- 장치와 서버는 같은 네트워크에 있어야 한다.
- HTTPS 는 기본 가정으로 두지 않는다.
- 브라우저로 열었을 때 실제 파일이 직접 다운로드되어야 한다.

### 7. 바이너리 이름 규칙

- `OtaUrl` 로 배포할 바이너리 이름은 가능하면 단순하게 유지한다.
- 공백, 복잡한 특수문자, 다단 경로는 피한다.
- 예:
  - `firmware.bin`
  - `firmware.bin.gz`
  - `smartplug.bin`
  - `smartplug.bin.gz`

### 8. 빌드 후 AI가 반드시 확인할 항목

AI는 빌드 후 아래를 항상 확인해야 한다.

1. 빌드 성공 여부
2. 산출물 경로
3. `.bin` 실제 크기
4. `.bin.gz` 크기 또는 압축 여부
5. 현재 크기 기준으로
   - 바로 OTA 가능
   - minimal 선행 필요
   둘 중 무엇인지

예시 형식:

- 산출물: `.pio/build/<env>/firmware.bin`
- 원본 크기: `671440 bytes`
- 압축 크기: `471757 bytes`
- 판단:
  - `.bin` 은 커서 minimal 선행 권장
  - `.bin.gz` 는 직접 OTA 시도 가능성 높음

### 9. 주의사항

- `tasmota-minimal.bin` 은 초기 설치용이 아니라 OTA 중간 단계용이다.
- `tasmota-minimal.bin` 만 올리고 끝내면 안 된다. 반드시 최종 full 펌웨어까지 이어서 올린다.
- 현재 장치 설정은 업그레이드 전에 백업하는 것을 기본 원칙으로 한다.
- 매우 오래된 Tasmota 버전에서 최신 버전으로 갈 때는 중간 업그레이드 경로가 필요할 수 있다.
- OTA 가 실패했다고 해서 바로 커스텀 OTA 로직을 추가하지 않는다. 먼저 Tasmota 기본 OTA 방식으로 해결 가능한지 검토한다.

### 10. 현재 프로젝트에 적용할 해석 기준

- 현재 프로젝트는 Tasmota 기본 Wi-Fi / OTA 를 그대로 유지해야 한다.
- 커스텀 코드는 MQTT/status/LED/장치 로직 위주만 유지한다.
- 빌드 결과 `firmware.bin` 크기가 500KB 를 넘으면 웹 UI 에서
  - `tasmota-minimal.bin(.gz)` 먼저 업그레이드
  - 그 다음 최종 `firmware.bin` 또는 `firmware.bin.gz`
  순서를 기본으로 한다.
- URL OTA 로 전환할 때도 원리는 같다. 다만 URL 은 **직접 바이너리 주소** 이어야 한다.
---

## UUID Topic Rule

This section is an exception rule for projects that must keep the legacy topic shape:

- `smart_plug/<uuid>/command`
- `smart_plug/<uuid>/status`

### 1. Default rule

If there is no legacy system requirement, Tasmota should keep using:

- `smart_plug/<mqtt_topic>/command`
- `smart_plug/<mqtt_topic>/status`

This remains the preferred Tasmota-native rule.

### 2. When UUID topics are required

If the existing broker, dashboard, backend, or client already requires
`smart_plug/<uuid>/...`, AI must not keep `mqtt_topic` as the device identifier.

In that case AI should switch the topic builder to a UUID-based identifier.

### 3. Do not reuse the original ESP-C3 UUID library directly

The original ESP-C3 code used a separate `Uuid` library and raw EEPROM storage.
That library is not assumed to exist inside Tasmota.

Therefore AI must not assume that:

- `uuid.begin()`
- `uuid.load()`
- raw EEPROM writes

can be copied into Tasmota unchanged.

### 4. Recommended UUID strategy inside Tasmota

If UUID topics are required, AI should use one of the following strategies in this order:

1. Use a fixed UUID string provided by the user or by project configuration.
2. Use a device-specific UUID string stored in Tasmota configuration.
3. Only if necessary, add first-boot UUID generation logic that stores a persistent value using a Tasmota-friendly settings path.

### 5. First implementation rule

For the first working port, AI should prefer the simplest stable method:

- define a fixed UUID string in `user_config_override.h`
- use that UUID when building MQTT topics

Example:

```c
#ifndef SMARTPLUG_UUID
#define SMARTPLUG_UUID "41742426-d035-4756-ba17-f542ea75ab02"
#endif
```

Then the topic builder should produce:

- `smart_plug/41742426-d035-4756-ba17-f542ea75ab02/command`
- `smart_plug/41742426-d035-4756-ba17-f542ea75ab02/status`

### 6. Multi-device rule

If the same firmware will be installed on more than one smart plug, AI must not leave a shared fixed UUID in place.

For multi-device deployments, AI must ensure each device gets a different persistent UUID.

Acceptable approaches:

- build one firmware per device with a different `SMARTPLUG_UUID`
- store a per-device UUID in Tasmota settings
- generate a per-device UUID on first boot and keep it persistent

### 7. What AI must explain to the user

When UUID topics are enabled, AI must explicitly tell the user:

- the exact UUID being used
- the exact command topic
- the exact status topic
- whether that UUID is shared by every build or unique per device

### 8. Safety rule

If AI cannot guarantee a unique UUID strategy for multiple devices, it must warn that topic collisions will happen.

### 9. Preferred multi-device implementation

For real smart plug deployments with more than one device, AI should prefer:

- generate a UUID on first boot
- store it in a persistent Tasmota settings slot
- reuse it on every later boot

Recommended implementation rule:

- use one `Mem` slot from Tasmota settings as the UUID storage
- validate the stored string
- if invalid or empty, generate a new UUID
- save it immediately
- use that value for `smart_plug/<uuid>/...`

Suggested default:

- use `SET_MEM16` or a clearly documented `SMARTPLUG_UUID_MEM_SLOT`

AI must also explain that if the selected `Mem` slot is already used by Rules or other project logic, a different slot must be chosen.
