# SMARTPLUG_PORTING_NOTES

이 파일은 `ESPC3ToTasmota설명서.md`와 분리된 프로젝트별 작업 노트다.
규칙 자체는 설명서에 두고, 이 파일에는 현재 스마트플러그 이식 과정에서 나온 사실과 실험 결과만 적는다.

## 1. 작업 목표

- 기존 ESP-C3/ESP 계열 Arduino 스타일 스마트플러그 코드를
- 공식 Tasmota 소스 구조로 옮겨
- Tasmota의 Wi-Fi, MQTT, OTA, WebUI, 설정 저장 기능은 그대로 사용하고
- 사용자 로직만 `xdrv` 형태로 붙인다.

## 2. 현재 Tasmota 커스텀 파일

- [platformio_override.ini](/c:/WS/vs_kdh/pnk_kdh/tasmota/Tasmota/platformio_override.ini)
- [platformio_tasmota_cenv.ini](/c:/WS/vs_kdh/pnk_kdh/tasmota/Tasmota/platformio_tasmota_cenv.ini)
- [user_config_override.h](/c:/WS/vs_kdh/pnk_kdh/tasmota/Tasmota/tasmota/user_config_override.h)
- [xdrv_98_smartplug_custom.ino](/c:/WS/vs_kdh/pnk_kdh/tasmota/Tasmota/tasmota/tasmota_xdrv_driver/xdrv_98_smartplug_custom.ino)

## 3. 원래 커스텀 펌웨어에서 확인된 값

- Wi-Fi SSID: `CC-Retail`
- Wi-Fi Password: `pnks1111`
- MQTT Host: `gym907-0001.iptime.org`
- MQTT Port: `1883`
- MQTT Username: `pnks`
- MQTT Password: `pnks1111`
- 버튼 GPIO 후보: `20`
- 릴레이 GPIO 후보: `4`
- LED GPIO 후보: `6`, `1`
- 명령 토픽 구조: `smart_plug/{uuid}/command`
- 상태 토픽 구조: `smart_plug/{uuid}/status`
- 상태 payload: `{"state":"on"}` / `{"state":"off"}`

주의:

- 위 GPIO는 원래 커스텀 코드에 있던 숫자이고, Tasmota Template GPIO와 1:1 대응이 보장되지 않는다.
- 실제 Tasmota 동작은 Template 또는 Module 설정으로 다시 맞춰야 한다.

## 4. OTA 실패 이력 요약

- 기존에는 커스텀 `FirmwareUpdater` 라이브러리로 서버에 버전 정보를 보내 OTA를 시도했다.
- 원복 과정에서 처음에는 서버가 버전 비교를 하는데, 올린 bin에 기대한 버전 체계가 없어서 안 되는 것으로 추정했다.
- 그래서 버전을 하나 올리고, 부팅 LED 점멸 속도도 바꿔서 OTA가 실제로 적용되는지 확인하려 했다.
- 하지만 장치에서 부팅 LED 패턴이 바뀌지 않았고 OTA도 되지 않았다.
- 정리하면, 기존 커스텀 OTA 경로는 현재 ESP8285/Tasmota 복구 흐름에서 신뢰하기 어렵다고 판단했다.

## 5. Tasmota 방향으로 바꾼 이유

- Tasmota 공식 문서 기준으로는 별도 앱을 완전히 새로 만드는 것보다
- Tasmota 기본 환경을 그대로 사용하고
- `user_config_override.h`, `platformio_tasmota_cenv.ini`, `xdrv_98_*.ino`
  형태로 기능을 추가하는 것이 권장 방식에 가깝다.

즉:

- Wi-Fi: Tasmota 기본 사용
- MQTT: Tasmota 기본 사용
- OTA: Tasmota 기본 사용
- 사용자 로직: `xdrv_98_smartplug_custom.ino`에 추가

## 6. 현재 구현된 것

- `smart_plug/<mqtt_topic>/command` 구독
- `{"cmd":"on"}`, `{"cmd":"off"}`, `{"cmd":"status"}` 처리
- 상태를 `smart_plug/<mqtt_topic>/status`로 publish
- 5초 주기 상태 publish
- 부팅 시 2초 동안 50ms LED 점멸
- OTA 진행 중 100ms LED 점멸

## 7. LED 관련 현재 판단

- Wi-Fi/MQTT 상태 LED는 Tasmota 기본 링크 LED 로직을 우선 사용한다.
- 부팅 시그니처와 OTA 중 빠른 점멸만 `xdrv_98_smartplug_custom.ino`에서 최소한으로 추가했다.
- 이 LED 동작이 실제로 보이려면 상태 LED가 Template에서 `LEDLNK` 또는 `LEDLNK_INV`로 잡혀 있어야 한다.
- Template이 안 맞으면 코드가 있어도 LED가 안 보일 수 있다.

## 8. 하드웨어 이슈

- Tasmota 스마트플러그 안의 ESP에 직접 업로드하려면 케이스를 열어야 할 가능성이 높다.
- 현재 뚜껑은 손으로 쉽게 열리지 않고, 접착 또는 끈적이 제거를 위해 열을 가해야 할 수 있다.
- 이 분해 여부와 안전성은 재진님과 추가 상의가 필요하다.

## 9. 현재 빌드 결과

- 커스텀 Tasmota env: `tasmota-smartplug`
- 최신 빌드 결과물:
  [firmware.bin](/c:/WS/vs_kdh/pnk_kdh/tasmota/Tasmota/.pio/build/tasmota-smartplug/firmware.bin)

## 10. 사용 원칙

- AI에게 변환 작업을 시킬 때는 먼저 [ESPC3ToTasmota설명서.md](/c:/WS/vs_kdh/pnk_kdh/tasmota/Tasmota/ESPC3ToTasmota설명서.md)를 기준으로 사용한다.
- 현재 스마트플러그에서 나온 특수 이력이나 추정은 이 노트 파일만 참고한다.
- 규칙을 바꿀 일이 아니면 설명서는 수정하지 않는다.
