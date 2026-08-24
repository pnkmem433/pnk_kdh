# 스마트 헹거/헹거랙 교체 기능 구현 완료 요약

## 구현 일자
2026-01-27

## 구현 완료 항목

### ✅ 1. 데이터베이스 스키마 변경
- **MySQL 마이그레이션 스크립트**: `docs/migration-add-rack-table.sql`
  - `rack` 테이블 생성 (붙을 행거랙)
  - `hangerleg` 테이블에 `rack_seq`, `position` 컬럼 추가
  - 외래키 제약조건 및 인덱스 추가
- **PostgreSQL CDC 마이그레이션 스크립트**: `docs/migration-add-rack-table-postgresql.sql`
  - MySQL과 동일한 스키마 변경을 PostgreSQL CDC에 적용

### ✅ 2. 엔티티 레이어
- **Rack 엔티티**: `src/entities/rack.entity.ts` 생성
  - `seq`, `rackNumber`, `rackLocation`, `createdAt` 필드
  - `OneToMany` 관계 (hangerlegs)
  
- **Hangerleg 엔티티**: `src/entities/hangerleg.entity.ts` 수정
  - `rackSeq`, `position` 필드 추가
  - `ManyToOne` 관계 (rack)

### ✅ 3. 모듈 생성
- **Rack 모듈**: `src/rack/` 디렉토리 생성
  - `rack.module.ts`: RackModule
  - `rack.service.ts`: RackService (findAll, findOne, findByRackNumber)
  - `rack.controller.ts`: RackController (GET /rack, GET /rack/:seq)

### ✅ 4. DTO 생성
- **ReplaceHangerDto**: `src/hanger/dto/replace-hanger.dto.ts`
  - `hangerUuid`: 스마트 헹거 UUID
  - `hangerlegUuid`: 스마트 헹거랙 UUID
  
- **ReplaceHangerlegDto**: `src/hangerleg/dto/replace-hangerleg.dto.ts`
  - `hangerlegUuid`: 스마트 헹거랙 UUID
  - `rackSeq`: 붙을 행거랙 시퀀스 번호 (선택사항)
  - `position`: 순서 (1, 2, 3, 4, 5...)

### ✅ 5. 서비스 레이어
- **HangerService.replaceHanger()**: `src/hanger/hanger.service.ts`
  - 헹거 UUID로 헹거 조회
  - 헹거랙 UUID로 헹거랙 조회하여 seq 획득
  - 헹거의 `hangerleg_seq` 업데이트
  - 에러 처리: NotFoundException
  
- **HangerlegService.replaceHangerleg()**: `src/hangerleg/hangerleg.service.ts`
  - 헹거랙 UUID로 스마트 헹거랙 조회
  - 붙을 행거랙 존재 확인
  - 같은 rack_seq 내에서 position 중복 체크
  - 헹거랙의 `rack_seq`, `position` 업데이트
  - 에러 처리: NotFoundException, BadRequestException

### ✅ 6. 컨트롤러 레이어
- **HangerController**: `src/hanger/hanger.controller.ts`
  - `PATCH /hanger/replace`: 스마트 헹거 교체 엔드포인트 추가
  
- **HangerlegController**: `src/hangerleg/hangerleg.controller.ts`
  - `PATCH /hangerleg/replace`: 스마트 헹거랙 교체 엔드포인트 추가

### ✅ 7. 모듈 의존성 설정
- **HangerModule**: HangerlegModule import 추가
- **HangerlegModule**: RackModule import 추가
- **AppModule**: Rack 엔티티 및 RackModule 추가

### ✅ 8. 전역 설정
- **main.ts**: ValidationPipe 전역 설정 추가
- **main.ts**: Swagger에 rack 태그 추가

### ✅ 9. 의존성 설치
- `class-validator`: DTO 검증용
- `class-transformer`: DTO 변환용

---

## API 엔드포인트

### 스마트 헹거 교체
```
PATCH /hanger/replace
Content-Type: application/json

{
  "hangerUuid": "8E4EF94C-6A44-4F70-6E14-BC410EB0F021",
  "hangerlegUuid": "1D947649930000"
}
```

### 스마트 헹거랙 교체
```
PATCH /hangerleg/replace
Content-Type: application/json

{
  "hangerlegUuid": "1D947649930000",
  "rackSeq": 1,
  "position": 3
}
```

### 붙을 행거랙 조회
```
GET /rack          # 모든 붙을 행거랙 조회
GET /rack/:seq     # 특정 붙을 행거랙 조회
```

---

## Docker 실행 방법

### Docker 이미지 빌드
```bash
docker build -t fxco-hmi:latest .
```

### Docker Compose로 실행 (개발 환경)
```bash
# .env 파일이 있는 상태에서 실행
docker-compose up -d

# 로그 확인
docker-compose logs -f

# 컨테이너 중지
docker-compose down
```

### Docker Compose로 실행 (프로덕션)
```bash
docker-compose -f docker-compose.prod.yml up -d
```

### 환경 변수 설정
- `.env` 파일을 프로젝트 루트에 생성해야 합니다
- `env.example` 파일을 참고하여 작성할 수 있습니다
- Docker Compose는 `env_file: - .env` 옵션으로 환경 변수를 자동으로 주입합니다

### 접속 정보
- **서버**: http://localhost:3000
- **Swagger 문서**: http://localhost:3000/api-docs
- **Health Check**: http://localhost:3000/health

---

## 다음 단계

### 1. 데이터베이스 마이그레이션 실행

#### MySQL 마이그레이션
```bash
# MySQL에 접속하여 마이그레이션 스크립트 실행
mysql -u [username] -p [database_name] < docs/migration-add-rack-table.sql
```

#### PostgreSQL CDC 마이그레이션
```bash
# PostgreSQL CDC 데이터베이스에 마이그레이션 스크립트 실행
psql -U [username] -d [database_name] -f docs/migration-add-rack-table-postgresql.sql

# 또는 psql 접속 후 실행
psql -U [username] -d [database_name]
\i docs/migration-add-rack-table-postgresql.sql
```

### 2. 초기 데이터 입력
- `rack` 테이블에 붙을 행거랙 데이터 입력 필요
- 기존 `hangerleg` 데이터에 `rack_seq`, `position` 값 설정 필요

### 3. 애플리케이션 실행

#### 로컬 개발 환경
```bash
npm install
npm run start:dev
```

#### Docker를 사용한 실행
```bash
# Docker 이미지 빌드
docker build -t fxco-hmi:latest .

# Docker Compose로 실행 (개발 환경)
docker-compose up -d

# 프로덕션 환경
docker-compose -f docker-compose.prod.yml up -d
```

**주의사항**: 
- `.env` 파일이 필요합니다 (환경 변수 설정)
- Docker Compose는 `env_file: - .env` 옵션으로 환경 변수를 주입합니다
- 데이터베이스 마이그레이션은 Docker 컨테이너 실행 전에 완료되어야 합니다

### 4. 테스트
- Swagger UI (`http://localhost:3000/api-docs`)를 통한 API 테스트
- Postman 또는 curl을 통한 통합 테스트

### 5. 에러 처리 확인
- 존재하지 않는 UUID로 요청 시 404 응답 확인
- 중복된 position으로 요청 시 400 응답 확인

---

## 주의사항

1. **데이터베이스 마이그레이션 필수**
   - 마이그레이션 스크립트를 실행하지 않으면 애플리케이션이 시작되지 않을 수 있습니다.

2. **기존 데이터 처리**
   - 기존 `hangerleg` 데이터는 `rack_seq`, `position`이 NULL일 수 있습니다.
   - NULL 허용으로 설정되어 있으므로 기존 데이터는 그대로 사용 가능합니다.

3. **Position 중복 체크**
   - 같은 `rack_seq` 내에서 `position`이 중복되지 않도록 검증이 구현되어 있습니다.
   - 중복 시 `BadRequestException` (400)이 발생합니다.

4. **ValidationPipe**
   - 전역 ValidationPipe가 설정되어 있어 DTO 검증이 자동으로 수행됩니다.
   - 잘못된 형식의 요청은 자동으로 400 에러를 반환합니다.

---

## 변경 사항 요약

### 🔍 Change Report

**무엇을 바꿨는가**
- 새로운 `rack` 테이블 및 엔티티 추가
- `hangerleg` 테이블에 `rack_seq`, `position` 컬럼 추가
- 스마트 헹거/헹거랙 교체 API 엔드포인트 추가
- Rack 모듈 (Service, Controller) 추가

**왜 바꿨는가**
- 스마트 헹거랙이 어느 붙을 행거랙에 설치되어 있는지 추적 필요
- 스마트 헹거랙의 순서(position) 관리 필요
- 스마트 헹거/헹거랙 교체 기능 구현 필요

**기존 대비 얻는 이점**
- 물리적인 행거랙 구조와 IoT 디바이스의 명확한 분리
- 위치 추적 및 관리 용이성 향상
- 교체 작업을 위한 API 제공

**운영 시스템으로 갈 때 예상 영향**
- 데이터 마이그레이션 필요
- 기존 데이터 호환성 유지 (NULL 허용)
- API 버전 관리 고려 필요

**되돌릴 수 있는지**
- Yes (마이그레이션 롤백 스크립트 작성 가능)
