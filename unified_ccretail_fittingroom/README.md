# unified_ccretail_fittingroom

## 목적
하나의 ESP에서 `CC-Retail_mvp0.0` 피팅룸 흐름을 끝까지 처리합니다.

- 도어 센서 전이 감지
- RFID 닫힘 윈도우 스캔
- LED MQTT publish
- CC-Retail API `door-status`, `session-items` 연동

## 필수 흐름
1. 도어 센서가 `OPEN` 전이되면 RFID를 중지하고 태그를 비웁니다.
2. 같은 시점에 LED MQTT로 `READY`를 publish 합니다.
3. 같은 시점에 `PATCH /api/fitting-rooms/{FITTING_ROOM_ID}/door-status` with `{"isDoorClosed":false}` 를 보냅니다.
4. 도어 센서가 `CLOSED` 전이되면 RFID를 시작하고 10초 스캔 윈도우를 엽니다.
5. 같은 시점에 LED MQTT로 `SCANNING`을 publish 합니다.
6. 같은 시점에 `PATCH /api/fitting-rooms/{FITTING_ROOM_ID}/door-status` with `{"isDoorClosed":true}` 를 보냅니다.
7. 닫힘 후 약 10초가 지나면 RFID를 중지합니다.
8. 동일 태그가 `RFID_MIN_READ_COUNT`회 이상 읽힌 경우만 남깁니다.
9. SKU는 기본적으로 `tag.substring(5, 13)`로 추출합니다.
10. 전송할 SKU가 있을 때만 `POST /api/fitting-rooms/{FITTING_ROOM_ID}/session-items` with `{"productVariantSkus":[...]}` 를 보냅니다.
11. POST 이후 LED MQTT로 `USING`을 publish 합니다.

## 도어 센서 논리
- 단일 센서만 사용합니다. 보조 도어 핀은 이 프로젝트에서 사용하지 않습니다.
- 기본 논리는 `HIGH=closed`, `LOW=open` 입니다.
- 배선이 다르면 `platformio.ini`에서 아래 매크로를 수정하세요.
- `DOOR_SENSOR_PIN`
- `DOOR_SENSOR_CLOSED_LEVEL`
- `DOOR_SENSOR_PIN_MODE`

## 주요 build_flags
- `API_HOST`, `API_PORT`
- `MQTT_HOST`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASS`
- `FITTING_ROOM_ID`
- `LED_STRIP_UUID`
- `DOOR_SENSOR_PIN`, `DOOR_SENSOR_CLOSED_LEVEL`, `DOOR_SENSOR_PIN_MODE`
- `DOOR_SCAN_WINDOW_MS`
- `RFID_MIN_READ_COUNT`
- `RFID_TAG_SUBSTRING_START`, `RFID_TAG_SUBSTRING_END`

## API 계약
- door status:
  `PATCH /api/fitting-rooms/{id}/door-status`
  body: `{"isDoorClosed":true|false}`
- session items:
  `POST /api/fitting-rooms/{id}/session-items`
  body: `{"productVariantSkus":["SKU1","SKU2"]}`

`lib/doorApi/doorApi.cpp`의 경로와 필드명은 그대로 유지합니다.

## SKU / DB 전제
- RFID에서 잘린 SKU 문자열이 CC-Retail DB의 `product_variant.sku`와 정확히 일치해야 합니다.
- 일치하지 않으면 `session-items`는 성공해도 원하는 상품 연결이 되지 않습니다.

## 네트워크 동작
- HTTP는 `UIPEthernet + EthernetClient`를 사용합니다.
- API 호출 직전에 LAN 연결을 다시 확인합니다.
- LAN이 죽어 있으면 `[HTTP] skip: LAN down` 로그를 남기고 요청을 생략합니다.
- MQTT는 LED publish 전용입니다. 도어 상태 subscribe는 사용하지 않습니다.
- `API_PORT=3007`은 현재 프로젝트 기본값일 뿐입니다. 실제 Nest 인스턴스가 다른 포트에서 `/api/fitting-rooms/*`를 서비스하면 그 포트로 바꿔야 합니다.

## 검증 로그
시리얼에서 아래 로그를 확인하세요.

- `[door] OPEN`
- `[door] CLOSED`
- `DoorApi: PATCH door-status ...`
- `Read tag: ...`
- `DoorApi: POST session-items ...`
- `[HTTP] Done status=...`

## 파일 메모
- 상태 머신: [src/main.cpp](C:/WS/vs_kdh/pnk_kdh/unified_ccretail_fittingroom/src/main.cpp)
- API 래퍼: [lib/doorApi/doorApi.cpp](C:/WS/vs_kdh/pnk_kdh/unified_ccretail_fittingroom/lib/doorApi/doorApi.cpp)
- 핀맵: [PINMAP.md](C:/WS/vs_kdh/pnk_kdh/unified_ccretail_fittingroom/PINMAP.md)
