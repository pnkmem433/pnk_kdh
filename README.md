# Plug Status Board

로컬 MQTT 기반 스마트 플러그 대시보드를 만들고 싶다.

이 대시보드는 ESP8685 기반 스마트 플러그를 Tasmota 커스텀 펌웨어로 구동하는 개발/테스트용 내부 대시보드다.

지금은 복잡한 기능보다, 장치를 한눈에 보고 상태를 직관적으로 확인하는 화면이 가장 중요하다.

중요:

이번 화면은 커스텀 MQTT 토픽만 기준으로 구성하면 된다.

즉 아래 토픽 구조만 사용해라.

- smart_plug/{uuid}/status

- smart_plug/{uuid}/metrics

- smart_plug/{uuid}/command

예시:

- smart_plug/3C0F021851A4/status = {"state":"off"}

- smart_plug/3C0F021851A4/metrics = {"state":"off","energy_available":false}

지금 단계에서 내가 원하는 화면 구성은 다음과 같다.

1. 대시보드 메인에 스마트 플러그들이 카드 형태로 보이게 한다.

2. 각 카드는 실제 “스마트 플러그” 느낌이 나도록 디자인한다.

3. 카드 중앙에는 중간 크기의 네모 박스가 있어야 한다.

4. 그 네모 박스 안에는 아래 정보가 들어가야 한다.

   - 플러그 이름

   - 이름 수정용 연필 아이콘 버튼

   - UUID

   - 현재 ON/OFF 상태

5. ON/OFF 상태는 글자만이 아니라 시각적으로도 바로 구분되게 한다.

   - ON이면 켜진 느낌

   - OFF면 꺼진 느낌

6. 플러그 이름은 사용자가 보기 쉬운 별칭이며, 나중에 수정 가능해야 한다.

7. UUID는 항상 보여야 한다.

8. 지금은 전력량, 전압, 로그, 상세 페이지 같은 기능은 넣지 않아도 된다.

9. 지금은 “장치가 보이고, 이름 보이고, uuid 보이고, 상태가 직관적으로 보이는 것”까지만 만들면 된다.

디자인 방향:

- 밝은 배경 기반

- 깔끔하고 현대적

- 너무 복잡하지 않게

- 실제 스마트 플러그 제품 카드처럼 보이게

- 카드 중앙 네모 박스가 핵심 정보 영역이 되게

- 연필 버튼은 이름 옆에 작고 직관적으로 배치

- ON/OFF 상태는 색상, 아이콘, 배경 강조 등으로 분명하게 표현

기술 방향:

- MQTT 데이터를 받아서 uuid 기준으로 카드 생성

- 각 uuid마다 하나의 플러그 카드

- status payload의 state 값을 기준으로 ON/OFF 표시

- 이름은 프론트엔드에서 임시로 편집 가능하게 해도 됨

- DB 없이도 됨

이건 상용 서비스가 아니라 개발/검증용 내부 대시보드이므로,

복잡한 기능보다 “한눈에 상태가 보이는 스마트 플러그 카드 UI”를 우선 설계해줘.

This project was built with [Lovable](https://lovable.dev).

**Live app**: https://smart-plug.lovable.app

## Build with Lovable

Continue developing this project in the [Lovable editor](https://lovable.dev/projects/c8d2162f-6887-4838-a952-210a0cd1f151).

- **Ship faster**: describe what you want to build and Lovable handles the code.
- **Stay in sync**: every change made in Lovable is committed straight to this repository.
- **Full ownership**: this code is yours. Push to `main` on GitHub and your changes sync back into Lovable, ready for your next prompt.

## Development

Prefer working locally? You need Node.js and npm — [install with nvm](https://github.com/nvm-sh/nvm#installing-and-updating).

```sh
git clone <this-repository-url>
cd <repository-name>
npm i
npm run dev
```
