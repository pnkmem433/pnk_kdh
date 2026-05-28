# unified_ccretail_fittingroom SETUP_GUIDE

## 준비물
- Seeed XIAO ESP32-C3
- RFID Reader FF704
- 단일 도어 센서 1개
- 상태 LED `D4`
- Ethernet 모듈
- LED strip 수신 보드 또는 MQTT 소비자

## 배선
- 도어 센서: `DOOR_SENSOR_PIN` default `GPIO2`
- RFID UART: `RX=D7`, `TX=D6`, `Serial1`, `115200`
- LAN CS: `D5`
- 상태 LED: `D4`

## 기본 도어 로직
- 기본값은 `HIGH=closed`, `LOW=open`
- 도어 센서 배선이 다르면 `DOOR_SENSOR_CLOSED_LEVEL`, `DOOR_SENSOR_PIN_MODE`를 바꾸세요.

## PlatformIO
1. `platformio.ini`에서 API, MQTT, fitting room, 도어 관련 build flag를 채웁니다.
2. 실제 CC-Retail Nest가 `3007`이 아니면 `API_PORT`를 수정합니다.
3. `pio run`
4. 업로드 후 시리얼 모니터 `115200`

## 기대 로그
- `[door] OPEN`
- `[door] CLOSED`
- `DoorApi: PATCH door-status ...`
- `Read tag: ...`
- `DoorApi: POST session-items ...`

## 자주 막히는 문제
- HTTP 404: `API_HOST:API_PORT`가 실제 `/api/fitting-rooms/*` 서버와 다를 가능성이 큽니다.
- LAN down: 배선 또는 DHCP 문제일 수 있으며 `[HTTP] skip: LAN down` 로그가 납니다.
- `session-items`가 비어 있음: 닫힘 10초 동안 같은 태그가 5회 이상 잡히지 않았거나 SKU 절단 범위가 DB와 다를 수 있습니다.
