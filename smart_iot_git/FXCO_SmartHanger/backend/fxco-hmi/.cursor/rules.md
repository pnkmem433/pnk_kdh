# FXCO-HMI 백엔드 프로젝트 Cursor Rules

## 프로젝트 개요

**프로젝트명**: FXCO HMI 백엔드 서버  
**목적**: 대구 FXCO 현장 스마트 행거 설치를 위한 HMI(Human Machine Interface) 백엔드 서버  
**기술 스택**: NestJS + TypeScript + TypeORM + MySQL  
**현재 단계**: 데모 → 운영 시스템 전환 중간 단계

---

## 1. 프로젝트 구조 규칙

### 1.1 디렉터리 구조

```
src/
├── main.ts                    # 애플리케이션 진입점
├── app.module.ts              # 루트 모듈
├── app.controller.ts          # 루트 컨트롤러 (Health Check)
├── entities/                  # 데이터베이스 엔티티 (TypeORM)
│   ├── hanger.entity.ts
│   ├── clothes.entity.ts
│   ├── hangerleg.entity.ts
│   ├── hanger-log.entity.ts
│   └── hmi-content.entity.ts
└── [domain]/                  # 도메인별 모듈
    ├── [domain].module.ts
    ├── [domain].controller.ts
    ├── [domain].service.ts
    └── dto/                   # Data Transfer Object
        └── [domain]-response.dto.ts
```

### 1.2 네이밍 규칙

- **엔티티**: PascalCase, 단수형 (예: `Hanger`, `Clothes`)
- **모듈**: PascalCase + Module (예: `HangerModule`)
- **서비스**: PascalCase + Service (예: `HangerService`)
- **컨트롤러**: PascalCase + Controller (예: `HangerController`)
- **DTO**: PascalCase + Dto (예: `HangerResponseDto`)
- **파일명**: kebab-case (예: `hanger.service.ts`)
- **테이블명**: snake_case (예: `hanger`, `hanger_log`)

---

## 2. 데이터베이스 및 엔티티 규칙

### 2.1 TypeORM 설정 원칙

- **synchronize: false** - 기존 테이블 사용, 자동 생성/수정 비활성화
- **엔티티는 데이터베이스 스키마를 반영**해야 함
- **컬럼명 매핑**: `@Column({ name: 'snake_case' })` 형식 사용
- **타입 매핑**: TypeORM 타입과 MySQL 타입 일치 필요

### 2.2 엔티티 작성 규칙

```typescript
@Entity('table_name')
export class EntityName {
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @Column({ name: 'column_name', type: 'varchar', length: 50, nullable: true })
  columnName: string;

  // 관계 정의
  @ManyToOne(() => RelatedEntity, { nullable: true })
  @JoinColumn({ name: 'foreign_key', referencedColumnName: 'seq' })
  relatedEntity: RelatedEntity;
}
```

### 2.3 ERD/SQL 변경 대응 원칙

**⚠️ 중요**: 데이터베이스 스키마 변경 시 다음을 반드시 확인:

1. **엔티티 파일 업데이트**
   - 컬럼 추가/삭제/수정 시 `entities/*.entity.ts` 수정
   - 타입 변경 시 TypeScript 타입도 함께 수정
   - 관계(Relation) 변경 시 `@ManyToOne`, `@OneToMany` 등 데코레이터 수정

2. **DTO 업데이트**
   - API 응답 구조 변경 시 `dto/*.dto.ts` 수정
   - Swagger 문서 자동 반영을 위해 `@ApiProperty` 데코레이터 유지

3. **서비스 로직 검토**
   - 쿼리 로직이 변경된 컬럼/관계를 사용하는지 확인
   - `relations` 옵션 업데이트 필요 여부 확인

4. **마이그레이션 고려사항**
   - TypeORM 마이그레이션 사용 시 `synchronize: false` 유지
   - 수동 마이그레이션 스크립트 작성 권장

---

## 3. API 설계 규칙

### 3.1 RESTful API 원칙

- **GET**: 조회 (Read)
- **POST**: 생성 (Create) - 현재 미구현
- **PUT/PATCH**: 수정 (Update) - 현재 미구현
- **DELETE**: 삭제 (Delete) - 현재 미구현

### 3.2 엔드포인트 패턴

```
GET    /[domain]              # 목록 조회
GET    /[domain]/:seq         # 단일 조회
POST   /[domain]              # 생성 (향후)
PUT    /[domain]/:seq         # 수정 (향후)
DELETE /[domain]/:seq         # 삭제 (향후)
```

### 3.3 Swagger 문서화

- 모든 엔드포인트에 `@ApiOperation` 데코레이터 필수
- 응답 타입에 `@ApiResponse` 데코레이터 필수
- DTO에 `@ApiProperty` 데코레이터 필수

---

## 4. 모듈 구조 규칙

### 4.1 모듈 생성 패턴

각 도메인은 독립적인 모듈로 구성:

```typescript
@Module({
  imports: [TypeOrmModule.forFeature([Entity])],
  controllers: [DomainController],
  providers: [DomainService],
  exports: [DomainService], // 다른 모듈에서 사용 가능하도록
})
export class DomainModule {}
```

### 4.2 의존성 주입

- **Repository**: `@InjectRepository(Entity)` 사용
- **Service**: 생성자 주입 사용
- **Cross-module**: `exports`를 통해 공유

---

## 5. 비즈니스 로직 규칙

### 5.1 서비스 레이어

- **비즈니스 로직은 Service에 위치**
- **Controller는 요청/응답 처리만 담당**
- **복잡한 쿼리는 Repository를 통해 처리**

### 5.2 데이터 조회 패턴

```typescript
// 기본 조회
async findAll(): Promise<Entity[]> {
  return await this.repository.find();
}

// 관계 포함 조회
async findOne(id: number): Promise<Entity> {
  return await this.repository.findOne({
    where: { seq: id },
    relations: ['relatedEntity'],
  });
}

// 커스텀 로직
async findWithCustomLogic(): Promise<Dto> {
  const entity = await this.repository.findOne(...);
  const additionalData = await this.getAdditionalData(...);
  return { ...entity, additionalData };
}
```

---

## 6. 환경 변수 규칙

### 6.1 필수 환경 변수

```env
DB_HOST=localhost
DB_PORT=3306
DB_USERNAME=root
DB_PASSWORD=your_password
DB_DATABASE=fxco_db
PORT=3000
NODE_ENV=development
```

### 6.2 환경 변수 사용

- `ConfigModule`을 통해 전역 설정
- `ConfigService`를 통해 주입받아 사용
- 기본값 제공 권장

---

## 7. 변경 관리 규칙 (Change Transparency)

### 7.1 변경 시 필수 제공 항목

구조, 패턴, 네이밍, 디렉터리 구성, 데이터 흐름 변경 시:

**🔍 Change Report (필수)**

1. **무엇을 바꿨는가**
2. **왜 바꿨는가**
3. **기존 대비 얻는 이점**
4. **운영 시스템으로 갈 때 예상 영향**
5. **되돌릴 수 있는지 (Yes / No)**

### 7.2 가정 선언 규칙

명시되지 않은 요구사항은 가정 가능하나, 반드시 명시적으로 선언:

```
"아래 구조는 [가정 내용]을 가정하고 설계되었습니다."
```

---

## 8. 데이터베이스 스키마 변경 대응 체크리스트

데이터베이스 스키마(ERD/SQL) 변경 시 다음 항목을 확인:

### 8.1 엔티티 레이어
- [ ] 엔티티 파일에 새 컬럼 추가/수정
- [ ] 컬럼 타입이 MySQL 타입과 일치하는지 확인
- [ ] `nullable` 옵션 확인
- [ ] 관계(Relation) 변경 시 데코레이터 업데이트
- [ ] `@JoinColumn`의 `name`과 `referencedColumnName` 확인

### 8.2 DTO 레이어
- [ ] API 응답에 포함되는 필드인지 확인
- [ ] DTO에 필드 추가/수정
- [ ] `@ApiProperty` 데코레이터 추가/수정

### 8.3 서비스 레이어
- [ ] 쿼리 로직이 변경된 컬럼을 사용하는지 확인
- [ ] `relations` 옵션 업데이트 필요 여부 확인
- [ ] 조인(Join) 로직 변경 필요 여부 확인

### 8.4 컨트롤러 레이어
- [ ] 엔드포인트 파라미터 변경 필요 여부 확인
- [ ] Swagger 문서 업데이트 필요 여부 확인

### 8.5 모듈 레이어
- [ ] `TypeOrmModule.forFeature`에 새 엔티티 추가 필요 여부 확인
- [ ] `app.module.ts`의 `entities` 배열 업데이트 필요 여부 확인

---

## 9. 현재 데이터베이스 스키마 요약

### 9.1 테이블 구조

**hanger**
- `seq` (INT, PK)
- `uuid` (VARCHAR(50))
- `clothes_seq` (INT, FK → clothes.seq)
- `hangerleg_seq` (INT, FK → hangerleg.seq)
- `nfc_active` (TINYINT)
- `last_pickdown_uuid` (VARCHAR(255))

**clothes**
- `seq` (INT, PK)
- `name` (VARCHAR)
- `tag` (VARCHAR)
- `media` (INT)
- `size_color_options` (JSON)
- `price` (INT)
- `web_site` (TEXT)

**hangerleg**
- `seq` (INT, PK)
- `uuid` (VARCHAR)

**hanger_log**
- `seq` (INT, PK)
- `hanger_seq` (INT, FK → hanger.seq)
- `hangerleg_seq` (INT, FK → hangerleg.seq)
- `created_at` (DATETIME)
- `self_written` (VARCHAR)

**hmi_content**
- `seq` (INT, PK)
- `title` (VARCHAR)
- `code` (VARCHAR)

### 9.2 관계 (Relations)

- `Hanger` → `Clothes` (ManyToOne)
- `HangerLog` → `Hanger` (ManyToOne, 간접 참조)
- `HangerLog` → `Hangerleg` (ManyToOne, 간접 참조)

---

## 10. 개발 워크플로우

### 10.1 새 기능 추가 시

1. 엔티티 확인/생성 (`entities/`)
2. 모듈 생성 (`[domain]/[domain].module.ts`)
3. 서비스 생성 (`[domain]/[domain].service.ts`)
4. 컨트롤러 생성 (`[domain]/[domain].controller.ts`)
5. DTO 생성 (`[domain]/dto/`)
6. `app.module.ts`에 모듈 등록
7. Swagger 문서화

### 10.2 데이터베이스 변경 시

1. SQL 스크립트 확인
2. 엔티티 파일 업데이트
3. DTO 업데이트 (필요 시)
4. 서비스 로직 검토
5. 테스트 실행
6. Change Report 작성

---

## 11. 테스트 규칙

- 단위 테스트: `*.spec.ts`
- E2E 테스트: `test/` 디렉터리
- 테스트 실행: `npm run test`, `npm run test:e2e`

---

## 12. 배포 규칙

### 12.1 Docker

- `Dockerfile` 사용
- `docker-compose.yml` (개발)
- `docker-compose.prod.yml` (프로덕션)
- 환경 변수는 런타임에 주입 (이미지에 포함하지 않음)

### 12.2 빌드

```bash
npm run build
npm run start:prod
```

---

## 13. 주의사항

1. **synchronize: false 유지** - 프로덕션 데이터베이스 보호
2. **환경 변수 보안** - `.env` 파일은 Git에 커밋하지 않음
3. **에러 핸들링** - 데이터베이스 연결 실패 시 재시도 로직 구현됨
4. **로깅** - 개발 환경에서만 SQL 쿼리 로깅 활성화

---

## 14. 참고 문서

- [NestJS 공식 문서](https://docs.nestjs.com/)
- [TypeORM 공식 문서](https://typeorm.io/)
- [프로젝트 README](../README.md)
- [개선 사항 문서](../docs/251218-improvements.md)
