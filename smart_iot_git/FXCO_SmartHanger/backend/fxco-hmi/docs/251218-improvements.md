# FXCO HMI 백엔드 서버 개선 사항 (2025-12-18)

## 프로젝트 개요

대구 FXCO 현장 스마트 행거 설치를 위한 HMI(Human Machine Interface) 백엔드 서버 개발 및 개선 작업을 진행했습니다.

---

## 주요 개선 사항

### 1. 프로젝트 초기 설정

#### 1.1 Git 저장소 초기화
- Git 저장소 초기화 완료
- `.gitignore` 파일 설정 (`.env`, `node_modules`, `dist` 등 제외)

#### 1.2 Nest.js 프로젝트 구조 생성
- Nest.js 백엔드 프레임워크 기반 프로젝트 구성
- TypeScript 설정 완료
- 기본 모듈 구조 생성

---

### 2. 데이터베이스 연동

#### 2.1 MySQL 데이터베이스 연결 설정
- TypeORM을 통한 MySQL 연결 구성
- ConfigModule을 사용한 환경 변수 관리
- `.env` 파일 기반 설정 (개발/프로덕션 분리)
- 연결 재시도 및 에러 핸들링 구현

#### 2.2 데이터베이스 엔티티 생성
다음 테이블에 대한 엔티티를 생성했습니다:

- **hanger** - 행거 정보
  - `seq` (INT, Primary Key)
  - `uuid` (VARCHAR(50))
  - `clothes_seq` (INT, Foreign Key → clothes.seq)
  - `hangerleg_seq` (INT, Foreign Key → hangerleg.seq)
  - `nfc_active` (TINYINT)
  - `last_pickdown_uuid` (VARCHAR(255))

- **clothes** - 의류 정보
  - `seq` (INT, Primary Key)
  - `name` (VARCHAR)
  - `tag` (VARCHAR)
  - `media` (INT)
  - `size_color_options` (JSON)
  - `price` (INT)
  - `web_site` (TEXT)

- **hangerleg** - 행거랙 정보
  - `seq` (INT, Primary Key)
  - `uuid` (VARCHAR)

- **hanger_log** - 행거 로그 정보
  - `seq` (INT, Primary Key)
  - `hanger_seq` (INT, Foreign Key → hanger.seq)
  - `hangerleg_seq` (INT, Foreign Key → hangerleg.seq)
  - `created_at` (DATETIME)
  - `self_written` (VARCHAR)

- **hmi_content** - HMI 콘텐츠 정보
  - `seq` (INT, Primary Key)
  - `title` (VARCHAR)
  - `code` (VARCHAR)

---

### 3. API 엔드포인트 구현

#### 3.1 Hanger API (`/hanger`)
- **GET `/hanger`** - 모든 행거 데이터 조회
  - `clothes` 정보 JOIN 조회
  - `hanger_log`와 연동하여 최근 `hangerleg_seq` 조회
  - `hangerlegSeqCurrent` 필드 추가 (현재 행거랙 위치 파악)

- **GET `/hanger/:seq`** - 특정 행거 데이터 조회
  - 동일한 기능 제공

**응답 예시:**
```json
{
  "seq": 1,
  "uuid": "524F0ACC-4414-41E6-A0CC-C8C97EF17A52",
  "clothesSeq": 1,
  "hangerlegSeq": 1,
  "nfcActive": 0,
  "lastPickdownUuid": null,
  "clothes": {
    "seq": 1,
    "name": "Jacket(CN11687)",
    "tag": "CN11687",
    "media": 2,
    "sizeColorOptions": [...],
    "price": 820000,
    "webSite": "https://..."
  },
  "hangerlegSeqCurrent": 1
}
```

#### 3.2 Clothes API (`/clothes`)
- **GET `/clothes`** - 모든 의류 데이터 조회
- **GET `/clothes/:seq`** - 특정 의류 데이터 조회

#### 3.3 Hangerleg API (`/hangerleg`)
- **GET `/hangerleg`** - 모든 행거랙 데이터 조회
  - UUID 기반 위치 파악을 위한 데이터 제공

- **GET `/hangerleg/:seq`** - 특정 행거랙 데이터 조회

**응답 예시:**
```json
[
  {
    "seq": 1,
    "uuid": "1D947649930000"
  },
  {
    "seq": 2,
    "uuid": "1D937649930000"
  }
]
```

#### 3.4 HMI Content API (`/hmi-content`)
- **GET `/hmi-content`** - 모든 HMI 콘텐츠 데이터 조회
  - `clothes.media`와 연동을 위한 데이터 제공

- **GET `/hmi-content/:seq`** - 특정 HMI 콘텐츠 데이터 조회

**응답 예시:**
```json
[
  {
    "seq": 1,
    "title": "GARDEN OF SPRING",
    "code": "GARDEN OF SPRING"
  },
  {
    "seq": 2,
    "title": "SHINING LIGHT",
    "code": "SHINING LIGHT"
  }
]
```

---

### 4. Swagger API 문서화

#### 4.1 Swagger 설정
- `@nestjs/swagger` 패키지 추가
- Swagger UI 경로: `http://localhost:3000/api-docs`
- 모든 API 엔드포인트에 대한 상세 문서화

#### 4.2 API 문서화 내용
- 각 엔드포인트의 요약 및 설명
- 요청/응답 스키마 정의
- 파라미터 설명
- 예시 데이터 제공

---

### 5. Hanger와 Hanger Log 연동

#### 5.1 기능 설명
- `hanger` 테이블 조회 시 `hanger_log` 테이블과 연동
- `hanger.seq`와 `hanger_log.hanger_seq`가 동일한 항목 중 가장 최근 데이터 조회
- 해당 데이터의 `hangerleg_seq`를 `hangerlegSeqCurrent`로 출력

#### 5.2 목적
- 픽업/픽다운 이벤트가 발생하지 않은 상태에서도 현재 행거의 위치를 파악하기 위함

#### 5.3 구현 방법
- `HangerService`에 `getCurrentHangerlegSeq()` 메서드 추가
- `hanger_log` 테이블에서 최신 데이터 조회 (seq DESC 정렬)
- `HangerResponseDto`에 `hangerlegSeqCurrent` 필드 추가

---

### 6. Docker 컨테이너화

#### 6.1 Dockerfile 생성
- 멀티 스테이지 빌드로 최적화된 이미지 생성
- Node.js 18 Alpine 기반 경량 이미지
- 프로덕션 의존성만 포함

#### 6.2 Docker Compose 설정
- `docker-compose.yml` - 개발 환경용
- `docker-compose.prod.yml` - 프로덕션 환경용
- 환경 변수 주입 설정

#### 6.3 보안 고려사항
- `.env` 파일은 이미지에 포함되지 않음
- `.dockerignore` 파일로 불필요한 파일 제외
- 런타임에 환경 변수 주입

---

### 7. 용어 통일

#### 7.1 한글 표기 변경
- "행거 다리" → "행거랙"으로 통일
- DB 테이블명(`hangerleg`)과 코드 변수명은 유지
- API 문서 및 설명에만 한글 표기 적용

---

## 기술 스택

- **Framework**: Nest.js 10.x
- **Language**: TypeScript 5.x
- **Runtime**: Node.js 18+
- **Database**: MySQL
- **ORM**: TypeORM 0.3.x
- **API Documentation**: Swagger/OpenAPI
- **Container**: Docker

---

## 프로젝트 구조

```
fxco-hmi/
├── src/
│   ├── main.ts                    # 애플리케이션 진입점
│   ├── app.module.ts              # 루트 모듈
│   ├── app.controller.ts          # 루트 컨트롤러
│   ├── entities/                  # 데이터베이스 엔티티
│   │   ├── hanger.entity.ts
│   │   ├── clothes.entity.ts
│   │   ├── hangerleg.entity.ts
│   │   ├── hanger-log.entity.ts
│   │   └── hmi-content.entity.ts
│   ├── hanger/                    # Hanger 모듈
│   │   ├── hanger.module.ts
│   │   ├── hanger.service.ts
│   │   ├── hanger.controller.ts
│   │   └── dto/
│   │       └── hanger-response.dto.ts
│   ├── clothes/                   # Clothes 모듈
│   ├── hangerleg/                 # Hangerleg 모듈
│   └── hmi-content/               # HMI Content 모듈
├── dist/                          # 빌드 출력
├── docs/                          # 문서
├── Dockerfile                     # Docker 이미지 빌드 파일
├── docker-compose.yml             # Docker Compose 설정 (개발)
├── docker-compose.prod.yml        # Docker Compose 설정 (프로덕션)
├── .dockerignore                  # Docker 제외 파일
├── .gitignore                     # Git 제외 파일
├── env.example                    # 환경 변수 예시
├── package.json
├── tsconfig.json
└── README.md
```

---

## 환경 변수 설정

`.env` 파일에 다음 변수를 설정해야 합니다:

```env
# 데이터베이스 설정
DB_HOST=127.0.0.1
DB_PORT=3306
DB_USERNAME=root
DB_PASSWORD=your_password
DB_DATABASE=cc_nanaland_mvp

# 서버 설정
PORT=3000
NODE_ENV=development
```

---

## 실행 방법

### 로컬 개발 환경
```bash
npm install
npm run start:dev
```

### Docker를 사용한 실행
```bash
# 이미지 빌드
docker build -t fxco-hmi:latest .

# Docker Compose로 실행
docker-compose up -d
```

### 프로덕션 환경
```bash
docker-compose -f docker-compose.prod.yml up -d
```

---

## API 접근

- **서버**: http://localhost:3000
- **Swagger 문서**: http://localhost:3000/api-docs
- **Health Check**: http://localhost:3000/health

---

## 주요 기능 요약

1. ✅ MySQL 데이터베이스 연동
2. ✅ Hanger, Clothes, Hangerleg, HMI Content 데이터 조회 API
3. ✅ Hanger와 Clothes JOIN 조회
4. ✅ Hanger와 Hanger Log 연동 (현재 위치 파악)
5. ✅ Swagger API 문서화
6. ✅ Docker 컨테이너화
7. ✅ 환경 변수 관리
8. ✅ 에러 핸들링 및 로깅

---

## 향후 개선 가능 사항

1. 인증/인가 기능 추가 (JWT 등)
2. 캐싱 전략 도입 (Redis 등)
3. 로깅 시스템 고도화
4. 단위 테스트 및 통합 테스트 추가
5. CI/CD 파이프라인 구축
6. 모니터링 및 알림 시스템 구축

---

## 작성일

2025년 12월 18일


