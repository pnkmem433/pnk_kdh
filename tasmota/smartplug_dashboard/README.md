# Smart Plug Dashboard

`smart_plug/+/status`, `smart_plug/+/metrics` 를 Node 백엔드가 MQTT로 구독하고, 프론트는 HTTP + SSE로 상태를 받아 그리는 대시보드입니다.

## 파일

- [index.html](/c:/WS/vs_kdh/pnk_kdh/tasmota/smartplug_dashboard/index.html)
- [server.js](/c:/WS/vs_kdh/pnk_kdh/tasmota/smartplug_dashboard/server.js)
- [package.json](/c:/WS/vs_kdh/pnk_kdh/tasmota/smartplug_dashboard/package.json)

## 구조

```text
스마트 플러그 -> MQTT 브로커
Node 백엔드 -> MQTT 구독
브라우저 -> /api/plugs, /api/events(SSE), /api/plugs/:uuid/command
```

## 기본 MQTT 설정

- Host: `api.pnkslab.com`
- Port: `1884`
- Username: `pnks`
- Password: `pnks1111`
- Topic Prefix: `smart_plug`

## 실행

```powershell
cd c:\WS\vs_kdh\pnk_kdh\tasmota\smartplug_dashboard
npm install
npm start
```

그 다음 브라우저에서 아래 주소로 접속합니다.

```text
http://localhost:8787
```

## 환경변수

필요하면 아래 값으로 덮어쓸 수 있습니다.

```powershell
$env:MQTT_HOST="api.pnkslab.com"
$env:MQTT_PORT="1884"
$env:MQTT_USER="pnks"
$env:MQTT_PASS="pnks1111"
$env:TOPIC_PREFIX="smart_plug"
$env:PORT="8787"
node server.js
```

## 현재 동작

- MQTT 구독
  - `smart_plug/+/status`
  - `smart_plug/+/metrics`
- 명령 발행
  - `smart_plug/{uuid}/command`
- 카드 자동 생성
  - 새로운 UUID가 들어오면 자동으로 생성
- 별칭 수정
  - 브라우저 `localStorage` 저장
- 상세 패널
  - UUID, 상태, 전압/전류/전력, 최근 원본 데이터 확인
