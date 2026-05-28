# PINMAP

## 단일 도어 센서 구성
- Door sensor: `DOOR_SENSOR_PIN` default `GPIO2`
- Door logic: default `HIGH=closed`, `LOW=open`
- Door pin mode: default `INPUT`

## 기타 핀
- LAN CS: `D5`
- RFID RX: `D7`
- RFID TX: `D6`
- Status LED: `D4`

## 주의
- 이 프로젝트는 단일 도어 센서만 사용합니다.
- `DOOR_SENSOR_B` 같은 보조 입력은 동작 흐름에 사용하지 않습니다.
- 배선이 다르면 `platformio.ini`의 도어 관련 build flag를 함께 수정하세요.
