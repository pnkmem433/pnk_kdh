# FXCO HMI 백엔드 서버

대구 FXCO 현장 스마트 행거 설치를 위한 HMI(Human Machine Interface) 백엔드 서버입니다.

## 프로젝트 정보

- **프로젝트명**: FXCO HMI
- **설치 일정**: 2024년 12월 22일
- **설치 장소**: 대구 FXCO 현장
- **기술 스택**: Nest.js (Node.js 백엔드 프레임워크)

## 기술 스택

- **Framework**: Nest.js
- **Language**: TypeScript
- **Runtime**: Node.js
- **Database**: MySQL
- **ORM**: TypeORM
- **API Documentation**: Swagger

## 설치 및 실행

### 필수 요구사항

- Node.js (v18 이상 권장)
- npm 또는 yarn
- MySQL (데이터베이스 서버)

### 설치

```bash
npm install
```

### 환경 변수 설정

1. `env.example` 파일을 참고하여 `.env` 파일을 생성합니다.

```bash
# .env 파일 생성
cp env.example .env
```

2. `.env` 파일에 데이터베이스 연결 정보를 입력합니다:

```env
DB_HOST=localhost
DB_PORT=3306
DB_USERNAME=root
DB_PASSWORD=your_password
DB_DATABASE=fxco_db
PORT=3000
NODE_ENV=development
```

### 개발 모드 실행

```bash
npm run start:dev
```

서버 실행 후 다음 URL에서 Swagger API 문서를 확인할 수 있습니다:
- **Swagger UI**: http://localhost:3000/api-docs

### 프로덕션 빌드

```bash
npm run build
npm run start:prod
```

## Docker 사용

### Docker 이미지 빌드

```bash
docker build -t fxco-hmi .
```

### Docker Compose로 실행 (개발 환경)

```bash
# .env 파일이 있는 상태에서 실행
docker-compose up -d
```

### Docker Compose로 실행 (프로덕션)

```bash
docker-compose -f docker-compose.prod.yml up -d
```

### 환경 변수 주입 방법

**중요**: `.env` 파일은 **도커 이미지에 포함되지 않습니다**. 보안을 위해 다음 방법 중 하나를 사용하세요:

1. **docker-compose.yml 사용** (개발 환경)
   - `env_file: - .env` 옵션으로 런타임에 주입

2. **환경 변수 직접 지정** (프로덕션)
   ```bash
   docker run -e DB_HOST=127.0.0.1 -e DB_PORT=3306 ... fxco-hmi
   ```

3. **환경 변수 파일 사용**
   ```bash
   docker run --env-file .env fxco-hmi
   ```

4. **Docker Secrets 사용** (프로덕션 권장)
   - Kubernetes, Docker Swarm 등에서 사용

## 프로젝트 구조

```
fxco-hmi/
├── src/
│   ├── main.ts              # 애플리케이션 진입점
│   ├── app.module.ts        # 루트 모듈
│   ├── app.controller.ts    # 루트 컨트롤러
│   ├── entities/            # 데이터베이스 엔티티
│   │   └── hanger.entity.ts # Hanger 테이블 엔티티
│   └── hanger/              # Hanger 모듈
│       ├── hanger.module.ts
│       ├── hanger.service.ts
│       └── hanger.controller.ts
├── dist/                    # 빌드 출력 디렉토리
├── test/                    # 테스트 파일
├── package.json
├── tsconfig.json
├── env.example              # 환경 변수 예시 파일
└── README.md
```

## API 엔드포인트

### Swagger API 문서

서버 실행 후 `http://localhost:3000/api-docs`에서 모든 API 엔드포인트를 확인하고 테스트할 수 있습니다.

### Hanger API

- `GET /hanger` - 모든 Hanger 데이터 조회
- `GET /hanger/:id` - 특정 ID의 Hanger 데이터 조회

### Health Check API

- `GET /` - 서버 상태 확인
- `GET /health` - 헬스 체크 (상세 정보)

## 개발 가이드

### 코드 스타일

```bash
npm run format
npm run lint
```

### 테스트

```bash
npm run test
npm run test:watch
npm run test:cov
npm run test:e2e
```

## 문서

- [개선 사항 문서 (2025-12-18)](./docs/251218-improvements.md)

## 라이선스

Private - UNLICENSED

