# HTML 모듈화 가이드

## 목적

`report_html` 아래의 보고서를 같은 규칙으로 계속 추가하고, 각 주제 폴더 안의 HTML만 기준으로 최신 보고서와 이전 기록을 관리한다.

핵심 원칙:

- 메인 홈과 주제별 기록 목록은 분리한다.
- 실제 보고서는 각 주제 폴더 안의 HTML 파일로만 관리한다.
- `최신 보고서`는 해당 폴더에서 `수정 날짜가 가장 최근인 HTML`을 가리킨다.
- `이전 보고서 기록`에는 해당 폴더의 모든 HTML 파일을 보여준다.
- `index.html`은 기록 목록 페이지이므로 기록 목록에는 포함하지 않는다.
- 루트의 `styling_booth.html`, `smart_plug.html`, `esp_now.html` 같은 파일은 보조/이전 자산일 수는 있지만, 기록 목록의 최신 판단 기준으로 쓰지 않는다.

---

## 최종 구조

```text
report_html/
├─ index.html
├─ AI_모듈화가이드.md
├─ report_registry.js
├─ archive_index.js
├─ common_section_nav.js
├─ common_auto_reload.js
├─ auto_deploy_report.ps1
├─ start_auto_deploy.bat
├─ styling_booth/
│  ├─ index.html
│  ├─ YYMMDD_이름_내용.html
│  └─ ...
├─ smart_plug/
│  ├─ index.html
│  ├─ YYMMDD_이름_내용.html
│  └─ ...
└─ esp_now/
   ├─ index.html
   ├─ YYMMDD_이름_내용.html
   └─ ...
```

---

## 페이지 역할

### 1. 메인 홈

파일:

- `report_html/index.html`

역할:

- 전체 보고서 주제 진입점
- 스타일링 부스 / 스마트 플러그 / ESP-NOW 목록으로 이동

### 2. 주제별 기록 목록

파일:

- `report_html/styling_booth/index.html`
- `report_html/smart_plug/index.html`
- `report_html/esp_now/index.html`

역할:

- 해당 주제의 아카이브 메인
- 최신 보고서 진입
- 이전 기록 펼쳐보기

### 3. 실제 기록 HTML

파일 예시:

- `report_html/styling_booth/260528_강동현_RFID테스트 결과 보고(phase1, phase2(옷1벌), phase2(옷여러벌)).html`
- `report_html/styling_booth/260528_강동현_RFID테스트케이스 구조화.html`

역할:

- 실제 본문
- 목록에서 클릭하면 바로 열리는 문서

---

## 파일명 규칙

기본 형식:

`YYMMDD_이름_내용.html`

예시:

- `260528_강동현_RFID테스트 결과 보고(phase1, phase2(옷1벌), phase2(옷여러벌)).html`
- `260528_강동현_RFID테스트케이스 구조화.html`
- `260528_강동현_RFID테스트 분석 방법.html`

구성:

1. 앞 6자리: 날짜
2. 그다음: 작성자 이름
3. 마지막: 문서 주제

제목을 따로 지정하지 않으면:

1. 오늘 날짜 `YYMMDD`
2. 작성자 이름
3. 문서의 큰 주제

순서로 만든다.

---

## 목록 표시 규칙

### 기록 제목

기록 목록 제목은 기본적으로 `파일명에서 .html만 제거한 값`을 그대로 사용한다.

예시:

- 파일명: `260528_강동현_RFID테스트케이스 구조화.html`
- 목록 제목: `260528_강동현_RFID테스트케이스 구조화`

### 오른쪽 날짜 배지

기록 목록 오른쪽 배지는 파일명 앞의 날짜를 읽어 `YYYY-MM-DD` 형식으로 보여준다.

예시:

- 파일명: `260528_강동현_RFID테스트케이스 구조화.html`
- 배지: `2026-05-28`

---

## 최신 보고서 규칙

최신 보고서는 아래 기준으로 정한다.

1. 해당 주제 폴더 안의 HTML 파일만 본다.
2. `index.html`은 제외한다.
3. `LastWriteTime`이 가장 최근인 파일을 최신으로 본다.

즉 파일명 순서가 아니라 수정 날짜가 기준이다.

---

## 이전 보고서 기록 규칙

`이전 보고서 기록`에는:

- 해당 폴더 안의 모든 HTML 파일을 보여준다.
- `index.html`은 제외한다.
- 최신 파일도 목록에 그대로 포함한다.

즉 구조는 항상 아래와 같다.

- 상단 최신 카드: 가장 최근 수정한 HTML
- 하단 기록 목록: 폴더 안의 모든 HTML

---

## 공통 HTML 형식

모든 보고서형 HTML은 최대한 같은 형식으로 만든다.

공통 요소:

- 상단 이동 링크
  - `기록 목록으로`
  - `메인 홈으로`
- 본문 섹션
- 오른쪽 고정 섹션 바로가기
- `맨 위로` 버튼
- 자동 새로고침 스크립트
- `body data-section-theme` 지정

오른쪽 섹션 메뉴는:

- `report_html/common_section_nav.js`

를 공통으로 사용한다.

스크립트가 `section` 내부의 `h1`, `h2`, `h3`를 읽어 자동으로 메뉴를 만든다.

즉 섹션 제목을 바꾸면 오른쪽 메뉴도 자동으로 같이 바뀐다.

자동 새로고침은:

- `report_html/common_auto_reload.js`

를 공통으로 사용한다.

이 스크립트는 HTTP로 띄운 페이지에서 파일 변경 시 새로고침만 담당한다.

즉 본문 공통화 도구가 아니라 `문서 새로고침 보조 기능`이다.

### 실제 보고서 HTML 필수 규칙

주제 폴더 안의 실제 보고서 HTML은 아래를 반드시 포함한다.

1. `body`에 `data-section-theme` 지정
   - 예: `data-section-theme="sky"`
2. 상단 링크 2개
   - `./index.html`로 가는 `기록 목록으로`
   - `../index.html`로 가는 `메인 홈으로`
3. 문서 하단 공통 스크립트 2개
   - `../common_section_nav.js`
   - `../common_auto_reload.js`

이 규칙은 `styling_booth/`, `smart_plug/`, `esp_now/` 내부의 실제 문서 HTML에 모두 동일하게 적용한다.

### 주제별 index.html 역할

`최신 문서`, `이전 문서 기록`, `최근 문서 보기` 같은 아카이브 기능은 실제 보고서 HTML이 아니라 주제별 `index.html`의 책임이다.

즉 역할은 아래처럼 나눈다.

- 실제 보고서 HTML: 본문 + 공통 이동 링크 + 섹션 네비 + 맨 위로
- 주제별 `index.html`: 최신 문서 카드 + 이전 문서 기록 목록

---

## 새 HTML 추가 규칙

예를 들어 사용자가:

`스타일링 부스에 새 HTML 기록 만들어줘`

라고 하면 처리 순서는 아래와 같다.

1. 주제 폴더 결정
   - 스타일링 부스: `report_html/styling_booth/`
   - 스마트 플러그: `report_html/smart_plug/`
   - ESP-NOW: `report_html/esp_now/`
2. 오늘 날짜 확인
3. 파일명 생성
   - `YYMMDD_이름_내용.html`
4. 해당 폴더 안에 HTML 생성
5. 기록 목록은 자동으로 이 파일을 읽어 표시
6. 수정 날짜가 가장 최근이면 최신 카드도 자동으로 그 파일을 가리킴

---

## 스타일링 부스 특이사항

스타일링 부스는 과거에 `data.js + styling_booth.html`처럼 분리된 구조를 사용한 적이 있다.

하지만 앞으로의 기준은 아래로 고정한다.

- 새 스타일링 부스 기록은 `styling_booth/` 폴더 안의 `단일 HTML`로 만든다.
- 새 보고서는 `data.js`와 본문 HTML을 분리하지 않는다.
- 보고서 1건은 가능한 한 `HTML 1파일`만으로 열리고 공유되게 만든다.
- 기록 목록과 최신 판단은 `styling_booth/` 폴더 안의 HTML만 기준으로 한다.
- 루트 `styling_booth.html`은 더 이상 운영 기준이 아니며, 남아 있더라도 기록 목록 연결에 의존하지 않는다.

즉 스타일링 부스도 다른 주제와 똑같이 `폴더 안 단일 HTML 중심`으로 운영한다.

---

## 배포 규칙

관련 파일:

- `report_html/start_auto_deploy.bat`
- `report_html/deploy_report_html.ps1`
- `report_html/report_registry.js`

동작:

1. 배포 직전에 `report_registry.js`를 현재 폴더 상태 기준으로 다시 생성
2. `report_html` 전체를 Vercel production으로 1회 배포

즉 지금은 자동 watcher 기반이 아니라, `배포가 필요할 때만 수동 실행`하는 방식이다.

수동 배포:

```cmd
C:\WS\vs_kdh\pnk_kdh\report_html\start_auto_deploy.bat
```

위 bat 파일은 이름은 그대로지만 실제 동작은 `자동 감시`가 아니라 `한 번만 전체 배포`다.

---

## 운영 요약

- 실제 보고서는 각 주제 폴더 안의 HTML만 기준으로 관리한다.
- 파일명 규칙은 `YYMMDD_이름_내용.html`
- 목록 제목은 파일명 그대로
- 오른쪽 배지는 날짜 표시
- 최신 보고서는 수정 날짜 기준
- 이전 기록은 해당 폴더 안의 모든 HTML
- 모든 보고서는 오른쪽 섹션 메뉴와 `맨 위로` 버튼을 공통 적용한다.

---

## HTML 생성 마스터 규칙 통합본

아래 내용은 기존 `html_rule.md`의 전체 규칙을 생략 없이 이 문서에 통합한 것이다. 앞으로 AI가 HTML 보고서를 만들거나 수정할 때는 이 섹션까지 포함한 현재 문서 전체를 단일 기준으로 사용한다.

### 기본 역할 정의

- 너는 최고 레벨의 프론트엔드 퍼블리셔이자, 시니어 총괄 책임자(사수)의 기획 관점을 100% 이해한 UI/UX 전문 AI다.
- 제공된 본문 마크다운 내용을 기반으로, 보고 성향 규칙과 UI/UX 기술 스펙을 완전히 융합한 단일 구조의 `index.html` 코드를 생성해야 한다.
- 결과물은 항상 이 문서 상단의 모듈 운영 규칙과 아래의 HTML 작성 규칙을 동시에 만족해야 한다.

### 1. 보고의 대원칙: 개발자 관점이 아닌 총괄자 관점

#### 1-1. 목적 중심 결과 기술

- 단순 하드웨어/소프트웨어 기능의 작동 여부만 적지 않는다.
- 예를 들어 통신 성공/실패 자체만 적는 식으로 끝내지 않는다.
- 실제 제품이 실사용 시나리오에서 어떻게 작동했고 어떤 의미가 있는지 총괄자 관점으로 해석해서 적는다.
- 예를 들어 스마트 행거, 스마트 플러그 같은 실제 사용 장면 기준으로 설명한다.

시나리오 기반 결과 도출 예시:

- `[X] 실사용 유지 패턴 안정성`: 옷을 걸어둔 장시간 대기 상태에서 NFC Read 유지가 무선 노이즈 간섭 없이 안정적으로 유지됨을 확인.
- `[X] 탈착 이벤트 즉시성`: 옷을 들어 올리는 순간 Remove 감지가 딜레이 없이 즉각 트리거됨을 확인.

#### 1-2. 최상단 고정 레이아웃

- 보고서의 시작은 긴 서론을 배제한다.
- 사수가 문서를 열자마자 전체 진행 상황과 핵심을 즉시 파악할 수 있어야 한다.
- 최상단은 반드시 두괄식 대시보드 구조로 배치한다.

최상단 필수 구성:

- `핵심 결론 (Conclusion)`: 해당 테스트/개발의 최종 요약과 성과를 최상단에 짧고 강하게 배치한다.
- `전체 시스템 구성도 / 워크플로우`: 세부 코드보다 입력, 처리, 출력 흐름이 보이는 구조도를 최상단에 둔다.

구성도 표현 방식:

- Mermaid.js
- 박스 그리드 형태
- 흐름도
- 다이어그램

#### 1-3. 중간점검 및 결과 분석 프레임워크

- `오늘의 할 일` 같은 작은 일일 작업 단위는 과감히 생략한다.
- 해당 기술 챕터의 전체 마일스톤 기준으로 성공과 실패를 분리해서 보여준다.

Status 구성 요소:

- `검증 완료 (Verified / Done)`
  - 성공적으로 시나리오 검증 및 구현이 완료된 큰 단위의 워크플로우를 정리한다.
  - 성공 근거 요약을 반드시 포함한다.
- `미결 / 해결 필요 (Pending / Blocked)`
  - 아직 구현되지 않았거나 문제가 발생한 파트를 적는다.
  - `구조적인 발생 원인(Why)`을 함께 적는다.
  - `앞으로의 해결 방향성(Action Item)`을 함께 적는다.

#### 1-4. 질문이 나오지 않는 근거 중심 데이터 배치

- 빈약한 데이터에 기반한 성급한 결론을 피한다.
- 사수가 질문하기 전에 방어 가능한 근거를 먼저 노출한다.
- 상단 카드나 결론 옆에 데이터 범위와 한계를 명확히 드러낸다.

필수 규칙:

- `테스트 스코프 명시`
  - 예: `자사 보유 샘플 5종(면, 패딩, 코트 등) 자체 테스트 완료`
  - 데이터의 한계와 범위를 명시한다.
- `외부 자료와 내 생각 분리`
  - 외부 기술 문서, 데이터시트, 학술 분석을 인용한 경우 본문 또는 카드에 출처 배지나 강조 표시를 둔다.
  - 예: `[참고/인용: 2.4GHz 주파수 섬유 투과율 분석 자료]`

#### 1-5. 미팅 및 시연 최적화 대시보드 UI

- 문서 링크를 열었을 때 과도한 스크롤 없이 최상단에서 핵심이 보여야 한다.
- 최상단에서 적어도 아래 흐름이 한눈에 들어와야 한다.

필수 흐름:

- `결론`
- `전체 구조`
- `통과/실패 상태`

- 텍스트 줄글은 최소화한다.
- 모든 데이터와 현황은 카드, 표, 그래프, 좌우 분할 레이아웃으로 빠르게 읽히게 구성한다.

### 2. Lovable 기반 UI/UX 기술 디자인 가이드

이 섹션은 HTML 시각 스타일과 출력 방식에 대한 고정 규칙이다.

#### 2-1. 기반 기술 설정

- CSS 프레임워크는 Tailwind CSS CDN을 사용한다.
- 예: `<script src="https://cdn.tailwindcss.com"></script>`
- 타이포그래피는 Google Fonts `Inter`를 적용한다.
- 전체 요소는 여유 있는 자간/행간을 유지한다.
- 예: `font-family: 'Inter', sans-serif; tracking-wide leading-relaxed`

#### 2-2. 색상 팔레트 및 테마 변수

- 배경은 연한 그레이/화이트 톤을 사용한다.
- 예: `#F1F5F9`
- 전경과 텍스트는 깊은 차콜 계열을 사용한다.
- 예: `#1E293B`
- 불필요하게 화려한 색 사용을 피하고 가독성을 우선한다.

상태별 컬러 코딩 카드 가이드:

- `검증 완료 (Verified / Done)`
  - `bg-[hsl(152,50%,94%)] text-[hsl(152,60%,30%)] border-[hsl(152,60%,42%)]`
- `진행 / 대기 (In Progress / Pending)`
  - `bg-blue-50 text-blue-700 border-blue-200`
- `이슈 / 블록 / 에러 (Blocked / Failed)`
  - `bg-red-50 text-red-700 border-red-200`
- `일반 카드 (Card)`
  - `bg-white`
  - `rounded-xl`
  - `shadow-sm`
  - `border border-slate-100`

#### 2-3. 시각적 구조화 및 도식화 요구사항

- 텍스트 나열식 구성은 금지한다.
- 데이터, 테스트 현황, 비교 정보는 표나 그리드 구조로 배치한다.
- 권장 레이아웃은 `grid grid-cols-1 md:grid-cols-2 gap-6` 형식의 좌우 분할이다.
- 정보의 연관성이 눈에 먼저 들어오도록 설계한다.

이미지 태그 처리 규칙:

- 본문에 `[사진: 파일명.jpg]` 마킹이 있으면 아래 형식의 HTML로 치환한다.

```html
<div class="my-6 flex flex-col items-center">
  <img src="./drive_images/파일명.jpg" class="w-full max-w-2xl rounded-xl shadow-md border border-slate-100">
  <p class="text-sm text-slate-500 text-center mt-2">▲ 사진 설명</p>
</div>
```

#### 2-4. 최종 출력 규격

- 마크다운 코드블록 기호인 ```` ```html ```` 같은 표시는 최종 HTML 결과물에 포함하지 않는다.
- 부연 설명, 챗봇식 인사말, 군더더기 텍스트를 붙이지 않는다.
- 사용자가 바로 파일로 저장할 수 있게 `<!DOCTYPE html>`로 시작하는 순수 HTML/CSS 원본만 반환한다.

---

## Lovable 디자인 시스템 연동 확장 규칙

이 섹션은 사용자가 전달한 실제 Lovable 소스코드 `app.css`, `index.tsx`, `table.tsx`, `accordion.tsx`, `package.json` 을 기준으로 `report_html`용 단일 HTML 생성 규칙에 접목한 확장 규약이다. 앞으로 AI가 새 HTML을 만들 때는 아래 규칙을 기존 운영 규칙과 함께 반드시 충족해야 한다.

### 0. 실제 소스 기준 선언

- 이 문서의 Lovable 연동 규칙은 추상적 취향이나 추정이 아니라, 사용자가 직접 전달한 실제 소스코드 구조를 기준으로 한다.
- CSS 시스템은 Tailwind v4 문법을 사용한다.
- 실제 소스에는 `@import "tailwindcss" source(none);`, `@source "../src";`, `@import "tw-animate-css";`, `@custom-variant dark (&:is(.dark *));` 가 포함된다.
- 따라서 React/Tailwind 원본을 설명할 때는 구형 `tailwind.config.js` 중심 사고보다 `@theme inline`, `:root`, `.dark`, `@layer base`, `@layer utilities` 중심 사고를 우선한다.
- 라우팅은 `@tanstack/react-router` 의 `createFileRoute("/")` 패턴을 따른다.
- 앱 구조는 `@tanstack/react-start`, `vite`, `tailwindcss 4`, `tw-animate-css`, `radix-ui`, `lucide-react`, `class-variance-authority`, `clsx`, `tailwind-merge` 조합을 사용하는 Lovable 계열 구조다.
- 아코디언은 `@radix-ui/react-accordion` 기반이며, 상태 애니메이션은 `tw-animate-css` 클래스와 `data-state` 속성으로 제어한다.
- 테이블은 무거운 데이터 그리드가 아니라 가벼운 래퍼 컴포넌트 구조를 유지한다.

### 1. 토큰 시스템 원칙

- 색상은 하드코딩한 RGB 나 임의 Hex 남발이 아니라 `oklch` 기반 토큰 변수 시스템으로 먼저 정의하고 사용한다.
- HTML 단일 파일로 생성하더라도 `:root` 아래에 색상, 배경, 카드, 보더, 포커스링, 상태색을 변수로 선언한 뒤 실제 컴포넌트는 이 변수를 참조한다.
- 실제 Lovable 소스의 핵심은 `@theme inline` 블록으로 CSS 변수와 Tailwind semantic utility 이름을 연결하는 방식이다.
- 예를 들어 `--color-background: var(--background)` 같은 매핑을 통해 `bg-background`, `text-foreground`, `border-border`, `bg-card` 같은 유틸리티가 semantic token 으로 작동한다.
- 새 semantic color 를 추가할 때는 반드시 아래 순서를 따른다.
1. `:root` 에 light value 를 추가한다.
2. `.dark` 에 dark value 를 추가한다.
3. `@theme inline` 에 `--color-<name>: var(--<name>)` 를 등록한다.
- 실제 소스 기준 기본 radius 체계는 `--radius` 하나를 기준으로 `sm`, `md`, `lg`, `xl`, `2xl`, `3xl`, `4xl` 를 계산 파생하는 구조다.
- 색상 시스템은 명도 대비가 확보된 중성 배경과 선명하지만 과하지 않은 상태 강조색 조합으로 설계한다.
- 보고서 페이지는 마케팅 랜딩처럼 화려한 원색 쇼케이스가 아니라, 장시간 읽어도 눈이 피로하지 않은 정보 밀도형 대시보드 톤을 유지한다.

실제 Lovable 방식의 토큰 매핑 예시:

```css
@theme inline {
  --radius-sm: calc(var(--radius) - 4px);
  --radius-md: calc(var(--radius) - 2px);
  --radius-lg: var(--radius);
  --radius-xl: calc(var(--radius) + 4px);
  --radius-2xl: calc(var(--radius) + 8px);
  --radius-3xl: calc(var(--radius) + 12px);
  --radius-4xl: calc(var(--radius) + 16px);
  --color-background: var(--background);
  --color-foreground: var(--foreground);
  --color-card: var(--card);
  --color-card-foreground: var(--card-foreground);
  --color-primary: var(--primary);
  --color-primary-foreground: var(--primary-foreground);
  --color-secondary: var(--secondary);
  --color-secondary-foreground: var(--secondary-foreground);
  --color-muted: var(--muted);
  --color-muted-foreground: var(--muted-foreground);
  --color-accent: var(--accent);
  --color-accent-foreground: var(--accent-foreground);
  --color-destructive: var(--destructive);
  --color-destructive-foreground: var(--destructive-foreground);
  --color-border: var(--border);
  --color-input: var(--input);
  --color-ring: var(--ring);
}
```

실제 소스에 맞춘 기준 토큰 예시:

```css
:root {
  --radius: 0.625rem;
  --background: oklch(1 0 0);
  --foreground: oklch(0.129 0.042 264.695);
  --card: oklch(1 0 0);
  --card-foreground: oklch(0.129 0.042 264.695);
  --popover: oklch(1 0 0);
  --popover-foreground: oklch(0.129 0.042 264.695);
  --primary: oklch(0.208 0.042 265.755);
  --primary-foreground: oklch(0.984 0.003 247.858);
  --secondary: oklch(0.968 0.007 247.896);
  --secondary-foreground: oklch(0.208 0.042 265.755);
  --muted: oklch(0.968 0.007 247.896);
  --muted-foreground: oklch(0.554 0.046 257.417);
  --accent: oklch(0.968 0.007 247.896);
  --accent-foreground: oklch(0.208 0.042 265.755);
  --destructive: oklch(0.577 0.245 27.325);
  --destructive-foreground: oklch(0.984 0.003 247.858);
  --border: oklch(0.929 0.013 255.508);
  --input: oklch(0.929 0.013 255.508);
  --ring: oklch(0.704 0.04 256.788);
}

.dark {
  --background: oklch(0.129 0.042 264.695);
  --foreground: oklch(0.984 0.003 247.858);
  --card: oklch(0.208 0.042 265.755);
  --card-foreground: oklch(0.984 0.003 247.858);
  --primary: oklch(0.929 0.013 255.508);
  --primary-foreground: oklch(0.208 0.042 265.755);
  --secondary: oklch(0.279 0.041 260.031);
  --secondary-foreground: oklch(0.984 0.003 247.858);
  --muted: oklch(0.279 0.041 260.031);
  --muted-foreground: oklch(0.704 0.04 256.788);
  --accent: oklch(0.279 0.041 260.031);
  --accent-foreground: oklch(0.984 0.003 247.858);
  --destructive: oklch(0.704 0.191 22.216);
  --destructive-foreground: oklch(0.984 0.003 247.858);
  --border: oklch(1 0 0 / 10%);
  --input: oklch(1 0 0 / 15%);
  --ring: oklch(0.551 0.027 264.364);
}
```

### 2. 타이포그래피 원칙

- 기본 본문 폰트는 항상 `font-sans`를 사용한다.
- 기본 자간은 Lovable 계열 `font-sans` 규칙을 따라 `letter-spacing: -0.01em`을 적용한다.
- 설명문, 카드 본문, 캡션, 상태 설명, 결론 요약은 모두 산세리프 기반으로 통일한다.
- RFID 태그 ID, UID, EPC, 테스트 카운트, 평균값, 분산 수치, 로그 파편, CSV형 데이터는 반드시 `font-mono`를 사용한다.
- 고정폭 데이터 셀은 `tabular-nums`에 준하는 정렬 인상을 주도록 우측 정렬 또는 중앙 정렬 정책을 함께 적용한다.
- 큰 대제목은 화려한 원색, 네온 계열, 그라데이션 텍스트를 절대 사용하지 않는다.
- 대제목은 `text-slate-900` 또는 순수 검은색 계열만 사용한다.
- 실제 base layer 에서는 `html` 에 `Inter`, `Noto Sans KR`, `Apple SD Gothic Neo`, `Malgun Gothic` 까지 포함한 폴백 체계를 둔다.
- 실제 base layer 에서는 `font-feature-settings: "cv11", "ss01", "ss03";`, `-webkit-font-smoothing: antialiased;`, `text-rendering: optimizeLegibility;` 를 함께 사용한다.
- 실제 base layer 에서는 `h1`, `h2`, `h3`, `h4` 에 `letter-spacing: -0.025em` 이 적용된다.
- 따라서 보고서 대제목도 본문보다 더 타이트한 자간을 사용해도 된다.
- 대제목의 권장 조합은 `text-4xl md:text-5xl font-black tracking-[-0.03em] text-slate-900`이다.

권장 베이스 유틸리티:

```css
html {
  font-family:
    "Inter", "Noto Sans KR", -apple-system, BlinkMacSystemFont, "Segoe UI",
    "Apple SD Gothic Neo", "Malgun Gothic", sans-serif;
  font-feature-settings: "cv11", "ss01", "ss03";
  -webkit-font-smoothing: antialiased;
  text-rendering: optimizeLegibility;
}

body {
  letter-spacing: -0.01em;
}

h1,
h2,
h3,
h4 {
  letter-spacing: -0.025em;
}

.data-mono {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
  font-variant-numeric: tabular-nums;
}
```

### 3. 배경 및 분위기 구성 원칙

- 배경은 평평한 단색 한 장으로 끝내지 않는다.
- 다만 과장된 마케팅 배경도 금지한다.
- 상단에는 매우 옅은 블루/화이트 또는 슬레이트 계열의 대형 블러 그라데이션을 넣고, 본문은 높은 가독성을 유지하는 밝은 중성 배경 위에 카드가 떠 있는 구조로 설계한다.
- 그라데이션은 항상 정보 가독성을 보조하는 수준이어야 하며, 텍스트 뒤에서 명암 충돌을 만들면 안 된다.
- 실제 소스의 semantic color token 은 `oklch` 로 정의되지만, `body` 배경 그래디언트 자체는 `rgba(...)` 와 `linear-gradient(...)` 조합으로 구현되어 있다.
- 즉 `모든 semantic color token 은 oklch`, `배경 연출용 gradient overlay 는 rgba 사용 가능` 으로 이해한다.
- 실제 base body 배경은 인디고, 스카이, 핑크 계열 radial overlay 세 개와 밝은 회청색 linear gradient 바탕을 겹친 구조다.

실제 Lovable 계열 배경 규격:

```css
body {
  background:
    radial-gradient(1200px 600px at 8% -10%, rgba(99, 102, 241, 0.14), transparent 60%),
    radial-gradient(900px 500px at 100% 0%, rgba(14, 165, 233, 0.12), transparent 55%),
    radial-gradient(700px 500px at 50% 110%, rgba(236, 72, 153, 0.08), transparent 60%),
    linear-gradient(180deg, #f6f7fb 0%, #eef2f8 100%);
  color: #0b1220;
}
```

### 4. 유틸리티 스타일 원칙

- 실제 소스에는 `shadow-soft`, `shadow-glow`, `glass`, `text-gradient-brand`, `ring-soft`, `hairline` 유틸리티가 있다.
- 보고서에서는 `glass`, `shadow-soft`, `ring-soft`, `hairline` 은 적극 사용 가능하다.
- `shadow-glow` 와 `text-gradient-brand` 는 프로덕트 브랜딩 UI 에는 쓸 수 있지만, 본 가이드의 보고서 대제목과 KPI 텍스트에는 기본적으로 사용 금지다.
- `glass` 류 유틸리티는 단순 반투명 박스가 아니라, 얕은 투명 백그라운드와 blur, saturate 로 정보층을 분리하는 용도로 쓴다.
- `shadow-soft` 류 유틸리티는 진한 검은 그림자 대신, 큰 반경과 낮은 알파값을 써서 카드가 부드럽게 떠 보이는 정도로만 사용한다.
- 과한 스큐모피즘, 깊은 그림자, 높은 채도의 컬러 글로우는 금지한다.

실제 소스 기반 유틸리티 예시:

```css
.shadow-soft {
  box-shadow:
    0 1px 0 rgba(15, 23, 42, 0.04),
    0 12px 24px -12px rgba(15, 23, 42, 0.12),
    0 24px 60px -28px rgba(30, 41, 99, 0.18);
}

.shadow-glow {
  box-shadow:
    0 1px 0 rgba(255, 255, 255, 0.7) inset,
    0 20px 40px -20px rgba(79, 70, 229, 0.35);
}

.glass {
  background: rgba(255, 255, 255, 0.72);
  backdrop-filter: saturate(1.2) blur(14px);
  -webkit-backdrop-filter: saturate(1.2) blur(14px);
}

.ring-soft {
  box-shadow: 0 0 0 1px rgba(15, 23, 42, 0.06), 0 1px 2px rgba(15, 23, 42, 0.04);
}

.hairline {
  background-image: linear-gradient(90deg, transparent, rgba(15, 23, 42, 0.12), transparent);
  height: 1px;
}
```

### 5. 레이아웃 강제 규칙

- 오른쪽 섹션 레이아웃 상시 유지는 선택사항이 아니라 필수다.
- 모니터 기준 메인 콘텐츠 영역은 항상 `좌측 컨트롤 패널 + 우측 데이터 인덱스`의 2영역 구조를 기본값으로 둔다.
- 좌측은 `lg:w-1/4` 비율의 대형 통계/대시보드 컨트롤 패널이다.
- 우측은 `flex-1` 비율의 마스터 테스트 데이터 인덱스 테이블 영역이다.
- 모바일에서는 세로 적층이 허용되지만, `lg` 이상에서는 반드시 좌우 병렬 구조가 유지되어야 한다.
- 좌측 패널은 KPI 카드, 테스트 스코프, 결론, 필터, 상태 범례, 실패 요약에 적합한 구조로 만든다.
- 우측 영역은 넓은 테이블, 검증 이력, 측정 로그, 아코디언 상세 행을 담당한다.

권장 레이아웃 예시:

```html
<div class="mx-auto flex max-w-[1800px] flex-col gap-6 px-4 py-8 lg:flex-row">
  <aside class="lg:w-1/4">
    <div class="glass shadow-soft ring-soft rounded-3xl border border-border p-6">
      <!-- KPI / conclusion / filters -->
    </div>
  </aside>

  <main class="flex-1 min-w-0">
    <section class="glass shadow-soft ring-soft rounded-3xl border border-border p-6">
      <!-- master test data index table -->
    </section>
  </main>
</div>
```

### 6. 제목 및 정보 위계 규칙

- 보고서 최상단 대제목은 `검은색 큰 제목 스타일`로 통일한다.
- 실제 소스에 `text-gradient-brand` 유틸리티가 존재하더라도, 보고서 메인 타이틀과 핵심 결론에는 사용하지 않는다.
- 절대 금지:
  - 무지개 그라데이션 제목
  - 파란색 대제목
  - 빨간색 대제목
  - 네온 포인트가 들어간 제목
  - `text-gradient-brand` 대제목
- 허용:
  - `text-slate-900`
  - `font-black`
  - 좁은 자간
  - 넉넉한 행간
  - 부제목에서만 muted 톤 사용

권장 구조:

```html
<header class="space-y-4">
  <p class="text-sm font-semibold uppercase tracking-[0.18em] text-slate-500">RFID Test Report</p>
  <h1 class="text-4xl font-black tracking-[-0.03em] text-slate-900 md:text-5xl">
    RFID 필수 태그셋 인식률 검증 리포트
  </h1>
  <p class="max-w-3xl text-base leading-7 text-slate-600">
    실사용 탈착 시나리오 기준으로 필수 태그셋 인식 안정성과 탈락 주범을 한 화면에서 식별한다.
  </p>
</header>
```

### 7. 테이블 컴포넌트 구조 규칙

- 마스터 데이터는 반드시 테이블 중심으로 보여준다.
- 헤더는 작고 단단하게, 바디는 넉넉한 행간과 명확한 구분선으로 구성한다.
- 표는 좁은 모바일 화면에서 `overflow-auto` 처리하되, 데스크톱에서는 가능한 한 한 화면에서 핵심 열이 모두 보이게 설계한다.
- 데이터 셀 내부 값은 의미에 따라 `font-sans` 와 `font-mono`를 구분한다.
- 헤더 셀은 muted 상단 라벨처럼 보이게 하되, 본문 값보다 더 튀면 안 된다.
- 실제 `table.tsx` 는 `Table`, `TableHeader`, `TableBody`, `TableFooter`, `TableRow`, `TableHead`, `TableCell`, `TableCaption` 구조를 제공한다.
- `Table` 은 `relative w-full overflow-auto` 래퍼 안에 `table.w-full.caption-bottom.text-sm` 를 넣는다.
- `TableHeader` 는 `[&_tr]:border-b`, `TableBody` 는 `[&_tr:last-child]:border-0`, `TableRow` 는 `border-b transition-colors hover:bg-muted/50 data-[state=selected]:bg-muted` 를 기본값으로 쓴다.
- `TableHead` 는 `h-10 px-2 text-left align-middle font-medium text-muted-foreground` 가 기본이고, `TableCell` 은 `p-2 align-middle` 이 기본이다.
- 보고서용 테이블은 이 기본 구조를 유지한 상태에서만 커스텀 스타일을 추가한다.

`table.tsx` 스타일을 반영한 권장 마크업 예시:

```html
<div class="relative w-full overflow-auto rounded-2xl border border-border bg-card">
  <table class="w-full caption-bottom text-sm">
    <thead class="[&_tr]:border-b bg-muted/40">
      <tr class="border-b transition-colors">
        <th class="h-10 px-2 text-left align-middle font-medium text-muted-foreground">라인</th>
        <th class="h-10 px-2 text-left align-middle font-medium text-muted-foreground">필수 태그셋</th>
        <th class="h-10 px-2 text-right align-middle font-medium text-muted-foreground">평균 인식 횟수</th>
        <th class="h-10 px-2 text-left align-middle font-medium text-muted-foreground">5회 미만 탈락 주범</th>
        <th class="h-10 px-2 text-left align-middle font-medium text-muted-foreground">판정</th>
      </tr>
    </thead>
    <tbody class="[&_tr:last-child]:border-0">
      <tr class="border-b transition-colors hover:bg-muted/50">
        <td class="p-2 align-middle font-medium text-slate-900">A-01</td>
        <td class="p-2 align-middle">
          <div class="flex flex-wrap gap-2">
            <span class="rounded-full bg-secondary px-2.5 py-1 text-xs font-medium text-secondary-foreground data-mono">E200341201</span>
            <span class="rounded-full bg-secondary px-2.5 py-1 text-xs font-medium text-secondary-foreground data-mono">E200341202</span>
          </div>
        </td>
        <td class="p-2 text-right align-middle text-slate-900 data-mono">6.8</td>
        <td class="p-2 align-middle text-muted-foreground">없음</td>
        <td class="p-2 align-middle">
          <span class="inline-flex items-center rounded-full border border-emerald-200 bg-emerald-50 px-2.5 py-1 text-xs font-semibold text-emerald-700">
            PASS
          </span>
        </td>
      </tr>
    </tbody>
  </table>
</div>
```

### 8. 아코디언 컴포넌트 구조 규칙

- 긴 테스트 근거, 로그, 시나리오 설명, 실패 원인 분석은 처음부터 전부 펼치지 않는다.
- 기본 화면은 요약 중심으로 두고, 상세 데이터는 아코디언으로 확장한다.
- 트리거는 제목, 상태, 요약 메타를 함께 포함하고, 열림/닫힘 아이콘이 부드럽게 회전하도록 만든다.
- 상세 본문은 표, 리스트, 코드 블록, 로그 박스를 담을 수 있게 여유 있는 패딩과 탑 보더를 둔다.
- 실제 `accordion.tsx` 는 `Accordion`, `AccordionItem`, `AccordionTrigger`, `AccordionContent` 구조를 사용한다.
- `AccordionItem` 의 기본 클래스는 `border-b` 다.
- `AccordionTrigger` 의 기본 클래스는 `flex flex-1 items-center justify-between py-4 text-sm font-medium cursor-pointer transition-all hover:underline text-left [&[data-state=open]>svg]:rotate-180` 이다.
- 아이콘은 `ChevronDown` 을 사용하고 `text-muted-foreground transition-transform duration-200` 으로 회전한다.
- `AccordionContent` 는 `overflow-hidden text-sm data-[state=closed]:animate-accordion-up data-[state=open]:animate-accordion-down` 을 기본으로 사용한다.
- 즉 접힘/펼침은 단순 `display:none` 이 아니라 `tw-animate-css` 기반 상태 애니메이션을 전제로 한다.

`accordion.tsx` 스타일을 반영한 권장 마크업 예시:

```html
<div class="rounded-2xl border border-border bg-card">
  <div class="border-b">
    <button
      type="button"
      class="flex w-full flex-1 items-center justify-between py-4 text-left text-sm font-medium transition-all hover:underline"
    >
      <span>
        <span class="block text-base font-semibold text-slate-900">라인 A-01 상세 측정 로그</span>
        <span class="mt-1 block text-sm text-muted-foreground">필수 태그셋 2종, 평균 6.8회, Remove 응답 정상</span>
      </span>
      <span class="text-muted-foreground transition-transform duration-200">⌄</span>
    </button>
    <div class="overflow-hidden text-sm">
      <div class="pb-4 pt-0">
        <div class="rounded-2xl bg-muted/40 p-4 text-sm leading-7 text-slate-700">
          세부 테스트 로그, 시나리오 메모, 실패 원인, 후속 액션을 이 영역에 배치한다.
        </div>
      </div>
    </div>
  </div>
</div>
```

### 9. TanStack Router 규격 반영 원칙

- 라우트 구조를 가진 React 프로젝트로 재사용할 수 있도록 문서 예시에는 `@tanstack/react-router` 규격을 기준으로 사고한다.
- 단일 HTML 보고서라도 정보 구조는 `route level layout` 개념을 따라 `page shell`, `section`, `data view`, `detail view`로 분리해서 작성한다.
- React 코드 예시가 필요한 경우 `createFileRoute` 패턴을 기준으로 설명한다.
- 페이지 진입 단위는 `index.tsx` 루트 라우트 사고방식을 따르며, 최상위에서 레이아웃을 먼저 잡고 내부에 데이터 컴포넌트를 주입하는 구조를 우선한다.
- 실제 `index.tsx` 는 `ReportPage` 컴포넌트를 루트 라우트에 연결한다.
- 또한 `head()` 함수 안에서 `title`, `description`, `og:title`, `og:description` 메타를 함께 정의한다.
- 따라서 보고서형 페이지는 화면 HTML 뿐 아니라 문서 메타 정보까지 함께 설계해야 한다.
- 정적 HTML 로 변환하는 경우에도 `<title>`, `<meta name="description">`, Open Graph 메타를 최대한 동일한 의미로 반영한다.

권장 라우트 사고 예시:

```tsx
import { createFileRoute } from "@tanstack/react-router";
import { ReportPage } from "@/components/ReportPage";

export const Route = createFileRoute("/")({
  head: () => ({
    meta: [
      { title: "RFID 테스트 결과 요약 — 강동현 phase1/phase2" },
      {
        name: "description",
        content: "RFID 스타일링 부스 테스트 결과 보고. 옷 1벌·여러벌 조건, 안테나 위치, 인식 간섭 요인 정리.",
      },
      { property: "og:title", content: "RFID 테스트 결과 요약" },
      {
        property: "og:description",
        content: "벽, 방향, 밀집, 사람, 안테나 위치가 인식 성능에 미치는 영향 분석.",
      },
    ],
  }),
  component: ReportPage,
});
```

### 10. RFID 마스터 테스트 테이블 알고리즘 규칙

- `5회 미만 빨간색 행 하이라이트 알고리즘`은 선택이 아니라 강제 규칙이다.
- 필수 태그셋 중 단 하나라도 실측 평균 인식 횟수가 `5` 미만이면 그 행 전체를 실패 행으로 처리한다.
- 실패 범위는 `0`, `1`, `2`, `3`, `4` 회다.
- 실패 행은 전체 배경을 `bg-rose-50/40`으로 반전시킨다.
- 실패 행의 텍스트 대비는 깨지지 않도록 기본 텍스트는 `text-slate-900` 유지, 실패 정보만 `rose` 계열 배지로 노출한다.
- `[5회 미만 탈락 주범]` 칸에는 미달 태그 ID를 하나 이상 표시한다.
- 미달 태그 ID는 각각 배지로 렌더링한다.
- 해당 배지 스타일은 `bg-rose-100 text-rose-700 font-mono`를 기본값으로 한다.
- 복수 미달 태그가 있으면 공백 나열이 아니라 배지 리스트로 출력한다.
- 모든 태그가 5회 이상이면 `[5회 미만 탈락 주범]` 칸에는 `없음` 또는 `-` 를 muted 톤으로 표시한다.
- 이 규칙은 표 렌더링 로직에서 판정, 행 배경, 실패 배지, 상태 배지를 동시에 연동해야 한다.

권장 렌더링 규격 예시:

```html
<tr class="border-b bg-rose-50/40 text-slate-900 transition-colors hover:bg-rose-50/70">
  <td class="p-2 align-middle font-medium">B-03</td>
  <td class="p-2 align-middle">
    <div class="flex flex-wrap gap-2">
      <span class="rounded-full bg-secondary px-2.5 py-1 text-xs font-medium text-secondary-foreground data-mono">E200341205</span>
      <span class="rounded-full bg-secondary px-2.5 py-1 text-xs font-medium text-secondary-foreground data-mono">E200341206</span>
      <span class="rounded-full bg-secondary px-2.5 py-1 text-xs font-medium text-secondary-foreground data-mono">E200341207</span>
    </div>
  </td>
  <td class="p-2 text-right align-middle data-mono">4.3</td>
  <td class="p-2 align-middle">
    <div class="flex flex-wrap gap-2">
      <span class="rounded-full bg-rose-100 px-2.5 py-1 text-xs font-semibold text-rose-700 data-mono">
        E200341206
      </span>
      <span class="rounded-full bg-rose-100 px-2.5 py-1 text-xs font-semibold text-rose-700 data-mono">
        E200341207
      </span>
    </div>
  </td>
  <td class="p-2 align-middle">
    <span class="inline-flex items-center rounded-full border border-rose-200 bg-rose-100 px-2.5 py-1 text-xs font-semibold text-rose-700">
      FAIL
    </span>
  </td>
</tr>
```

판정용 의사코드:

```js
const underFiveTags = requiredTags.filter((tag) => tag.avgReadCount < 5);
const isFailedRow = underFiveTags.length > 0;

const rowClassName = isFailedRow
  ? "bg-rose-50/40 text-slate-900 hover:bg-rose-50/70"
  : "hover:bg-muted/50";
```

### 11. 섹션 배치 규칙

- 첫 화면에는 최소한 아래 블록이 위에서 아래 순서로 보여야 한다.
- `검은색 대제목`
- `한 줄 결론 또는 결론 카드`
- `테스트 스코프 / Verified / Pending 요약 카드`
- `좌측 컨트롤 패널 + 우측 마스터 테이블`
- `아코디언 상세 로그`
- 표보다 먼저 긴 배경 설명을 배치하지 않는다.
- 핵심 숫자와 판정이 먼저 보여야 한다.
- 배경 설명은 반드시 카드 또는 아코디언 안으로 집어넣어 스캔성을 해치지 않게 한다.

### 12. HTML 단일 파일 작성 시 권장 head 규격

- `Tailwind CDN`
- `Google Fonts Inter`
- `Mermaid`
- 커스텀 토큰용 `style` 블록
- 실제 Lovable 프로젝트 원형은 Tailwind v4 와 Vite 기반이지만, `report_html` 단일 파일은 빌드 없는 HTML 이므로 CDN 방식으로 번안한다.
- 단, 번안하더라도 naming 철학은 `background`, `foreground`, `card`, `muted`, `border`, `ring` 같은 semantic token 구조를 그대로 유지한다.

권장 예시:

```html
<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RFID 테스트 보고서</title>
  <meta name="description" content="RFID 스타일링 부스 테스트 결과 보고. 옷 1벌·여러벌 조건, 안테나 위치, 인식 간섭 요인 정리.">
  <meta property="og:title" content="RFID 테스트 결과 요약">
  <meta property="og:description" content="벽, 방향, 밀집, 사람, 안테나 위치가 인식 성능에 미치는 영향 분석.">
  <script src="https://cdn.tailwindcss.com"></script>
  <script>
    tailwind.config = {
      theme: {
        extend: {
          fontFamily: {
            sans: ["Inter", "Noto Sans KR", "sans-serif"],
            mono: ["ui-monospace", "SFMono-Regular", "Menlo", "monospace"],
          },
          boxShadow: {
            soft: "0 1px 0 rgba(15, 23, 42, 0.04), 0 12px 24px -12px rgba(15, 23, 42, 0.12), 0 24px 60px -28px rgba(30, 41, 99, 0.18)",
          },
        },
      },
    };
  </script>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800;900&family=Noto+Sans+KR:wght@400;500;700;900&display=swap" rel="stylesheet">
  <script src="https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js"></script>
  <style>
    :root {
      --radius: 0.625rem;
      --background: oklch(1 0 0);
      --foreground: oklch(0.129 0.042 264.695);
      --card: oklch(1 0 0);
      --card-foreground: oklch(0.129 0.042 264.695);
      --muted: oklch(0.968 0.007 247.896);
      --muted-foreground: oklch(0.554 0.046 257.417);
      --border: oklch(0.929 0.013 255.508);
    }
    html {
      font-family:
        "Inter", "Noto Sans KR", -apple-system, BlinkMacSystemFont, "Segoe UI",
        "Apple SD Gothic Neo", "Malgun Gothic", sans-serif;
      font-feature-settings: "cv11", "ss01", "ss03";
      -webkit-font-smoothing: antialiased;
      text-rendering: optimizeLegibility;
    }
    body {
      letter-spacing: -0.01em;
      background:
        radial-gradient(1200px 600px at 8% -10%, rgba(99, 102, 241, 0.14), transparent 60%),
        radial-gradient(900px 500px at 100% 0%, rgba(14, 165, 233, 0.12), transparent 55%),
        radial-gradient(700px 500px at 50% 110%, rgba(236, 72, 153, 0.08), transparent 60%),
        linear-gradient(180deg, #f6f7fb 0%, #eef2f8 100%);
      color: #0b1220;
    }
    h1, h2, h3, h4 {
      letter-spacing: -0.025em;
    }
    .glass {
      background: rgba(255, 255, 255, 0.72);
      backdrop-filter: saturate(1.2) blur(14px);
      -webkit-backdrop-filter: saturate(1.2) blur(14px);
    }
    .data-mono {
      font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
      font-variant-numeric: tabular-nums;
    }
  </style>
</head>
```

### 13. 실제 보고서 HTML 체크리스트

- `body data-section-theme`가 있는가
- 상단에 `기록 목록으로`, `메인 홈으로` 링크가 있는가
- 최상단 대제목이 `text-slate-900` 검은색 헤비 볼드인가
- 본문 기본 폰트가 `font-sans` 및 `letter-spacing: -0.01em` 규칙을 따르는가
- `html` 레벨 폰트 폴백과 `font-feature-settings` 적용 여부를 고려했는가
- RFID 태그 ID 및 수치 셀이 `font-mono`인가
- 좌측 `lg:w-1/4` 대시보드 패널과 우측 `flex-1` 마스터 테이블 레이아웃이 유지되는가
- 테이블이 마스터 데이터 인덱스 역할을 수행하는가
- 5회 미만 태그 존재 시 행 전체가 `bg-rose-50/40`로 하이라이트되는가
- `[5회 미만 탈락 주범]` 칸에 미달 태그 ID 배지가 노출되는가
- 상세 근거가 아코디언 또는 별도 세부 카드로 접히는가
- 아코디언 open/close 상태가 `data-state` 또는 동등한 구조로 자연스럽게 제어되는가
- 문서 메타 정보 `title`, `description`, `og:title`, `og:description` 를 함께 설계했는가
- `../common_section_nav.js` 와 `../common_auto_reload.js`가 포함되는가

### 14. 금지 규칙

- 대제목에 그라데이션 텍스트 사용 금지
- 실제 유틸리티에 `text-gradient-brand` 가 존재하더라도 보고서 메인 타이틀에는 사용 금지
- 본문 전체를 원색 포인트 컬러 위주로 구성하는 것 금지
- 데이터 표 안에서 비례폭 폰트로 UID / EPC / 숫자를 섞어 쓰는 것 금지
- 핵심 판정이 표 아래로 밀리는 긴 서론형 레이아웃 금지
- 실패 행을 텍스트만 빨갛게 처리하고 행 배경을 유지하지 않는 것 금지
- 상세 로그를 모두 펼쳐놓아 첫 화면 스캔성을 해치는 것 금지
- 좌우 2영역 구조를 데스크톱에서 단일 컬럼으로 붕괴시키는 것 금지

### 15. 실제 생성 시 AI 행동 규칙

- 새 보고서를 만들 때 AI는 먼저 주제 폴더를 선택한다.
- 이어서 파일명 규칙 `YYMMDD_이름_내용.html`로 저장 대상을 결정한다.
- 문서 헤드에는 Lovable 스타일 토큰과 Tailwind CDN 구성을 먼저 넣는다.
- 본문 첫 화면은 반드시 `검은색 대제목 + 결론 + 상태 카드 + 좌측 패널 + 우측 테이블` 순서로 설계한다.
- 표 데이터가 있다면 5회 미만 하이라이트 알고리즘을 우선 적용한다.
- 긴 근거 데이터는 아코디언으로 접는다.
- React 라우트 원형을 만들 때는 `createFileRoute("/")`, `head()`, `component` 구조를 기본 뼈대로 삼는다.
- 아코디언과 테이블은 임의 div 조합으로 새로 발명하지 말고, 실제 `accordion.tsx` 와 `table.tsx` 의 계층 구조를 먼저 따른다.
- 마지막에 공통 스크립트와 상단 이동 링크 규칙을 점검한다.

---

## 이 문서를 실제로 사용하는 방법

- `report_html/AI_모듈화가이드.md`만 단일 기준 문서로 사용한다.
- 기존 `html_rule.md`를 따로 보지 않아도 되도록 이 문서에 운영 규칙과 HTML 생성 규칙을 함께 유지한다.
- 새 보고서를 만들 때는 먼저 상단의 폴더 운영 규칙을 따른다.
- 이어서 이 문서 하단의 `HTML 생성 마스터 규칙 통합본`과 `Lovable 디자인 시스템 연동 확장 규칙`에 맞춰 화면 구조와 문장 톤을 작성한다.

## 최종 통합 요약

- 현재 문서는 `report_html` 운영 방식 규칙과 `html_rule.md`의 HTML 작성 규칙을 모두 포함한다.
- 보고서 파일 관리 기준은 `주제 폴더 내부의 단일 HTML` 중심이다.
- 보고서 화면 설계 기준은 `총괄자 관점`, `두괄식 대시보드`, `근거 중심 데이터 배치`, `Lovable 스타일`이다.
- Lovable 연동 확장 규칙에는 `oklch 토큰 시스템`, `font-sans 자간 -0.01em`, `glass / shadow-soft`, `TanStack Router 사고방식`, `table / accordion 마크업 패턴`, `RFID 5회 미만 하이라이트 알고리즘`이 포함된다.
- 앞으로 AI에게 HTML 생성이나 수정 작업을 요청할 때는 이 문서 하나만 기준으로 사용하면 된다.


---

## 한글 인코딩 규칙

한글이 들어간 HTML, Markdown, JavaScript 문자열은 모두 **UTF-8로 저장**한다.

필수 규칙:

- HTML 문서에는 반드시 `<meta charset="UTF-8" />`를 넣는다.
- PowerShell로 파일을 저장할 때는 기본 인코딩에 맡기지 말고 `Set-Content -Encoding UTF8` 또는 `Out-File -Encoding utf8`을 명시한다.
- 한글 문장을 문자열로 대량 치환할 때는 `Get-Content -Raw -Encoding UTF8`로 읽고, 저장도 UTF-8로 다시 쓴다.
- 터미널에서 글자가 깨져 보여도 브라우저와 IDE에서 정상일 수 있으므로, 최종 확인은 **브라우저 + IDE** 기준으로 한다.
- `?곷떒...`, `諛곗튂...` 같은 형태가 보이면 내용 자체가 틀린 것이 아니라 **인코딩이 깨진 것**일 가능성이 크므로, 원문 한국어를 다시 넣고 UTF-8로 저장한다.

권장 작업 순서:

1. 원문 한국어 문장을 먼저 확정한다.
2. 파일을 UTF-8로 읽는다.
3. 문자열 치환 또는 편집을 수행한다.
4. UTF-8로 다시 저장한다.
5. 브라우저와 IDE에서 제목, 본문, 버튼 라벨 한글이 정상인지 확인한다.
