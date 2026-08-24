# 데이터베이스 스키마 변경 대응 분석 문서

## 목적

이 문서는 FXCO-HMI 백엔드 프로젝트에서 데이터베이스 스키마(ERD/SQL)가 변경될 때, 코드베이스의 어떤 부분을 어떻게 수정해야 하는지 체계적으로 정리한 가이드입니다.

---

## 1. 현재 데이터베이스 스키마 구조

### 1.1 테이블 목록 및 관계

```
hanger (행거)
├── seq (PK)
├── uuid
├── clothes_seq (FK → clothes.seq)
├── hangerleg_seq (FK → hangerleg.seq)
├── nfc_active
└── last_pickdown_uuid

clothes (의류)
├── seq (PK)
├── name
├── tag
├── media
├── size_color_options (JSON)
├── price
└── web_site

hangerleg (행거랙)
├── seq (PK)
└── uuid

hanger_log (행거 로그)
├── seq (PK)
├── hanger_seq (FK → hanger.seq)
├── hangerleg_seq (FK → hangerleg.seq)
├── created_at
└── self_written

hmi_content (HMI 콘텐츠)
├── seq (PK)
├── title
└── code
```

### 1.2 관계 (Relations)

- **Hanger → Clothes**: ManyToOne (hanger.clothes_seq → clothes.seq)
- **HangerLog → Hanger**: ManyToOne (hanger_log.hanger_seq → hanger.seq)
- **HangerLog → Hangerleg**: ManyToOne (hanger_log.hangerleg_seq → hangerleg.seq)

---

## 2. 스키마 변경 시 영향 범위 분석

### 2.1 변경 유형별 영향도 매트릭스

| 변경 유형 | 엔티티 | DTO | 서비스 | 컨트롤러 | 모듈 | 영향도 |
|---------|:-----:|:---:|:-----:|:--------:|:---:|:-----:|
| 컬럼 추가 (nullable) | ✅ | ⚠️ | ⚠️ | ⚠️ | ❌ | 낮음 |
| 컬럼 추가 (not null) | ✅ | ✅ | ✅ | ⚠️ | ❌ | 중간 |
| 컬럼 삭제 | ✅ | ✅ | ✅ | ⚠️ | ❌ | 높음 |
| 컬럼 타입 변경 | ✅ | ✅ | ⚠️ | ⚠️ | ❌ | 중간 |
| 컬럼명 변경 | ✅ | ✅ | ⚠️ | ⚠️ | ❌ | 중간 |
| PK 변경 | ✅ | ✅ | ✅ | ✅ | ❌ | 매우 높음 |
| FK 추가 | ✅ | ⚠️ | ✅ | ⚠️ | ⚠️ | 중간 |
| FK 삭제 | ✅ | ✅ | ✅ | ⚠️ | ⚠️ | 높음 |
| FK 관계 변경 | ✅ | ✅ | ✅ | ⚠️ | ⚠️ | 높음 |
| 새 테이블 추가 | ✅ | ✅ | ✅ | ✅ | ✅ | 중간 |
| 테이블 삭제 | ✅ | ✅ | ✅ | ✅ | ✅ | 매우 높음 |

**범례**: ✅ 필수 수정, ⚠️ 조건부 수정, ❌ 수정 불필요

---

## 3. 변경 시나리오별 상세 가이드

### 3.1 시나리오 1: 컬럼 추가 (nullable)

**예시**: `hanger` 테이블에 `status` 컬럼 추가 (VARCHAR(50), nullable)

#### 수정 파일 목록

1. **엔티티 파일**: `src/entities/hanger.entity.ts`
   ```typescript
   @Column({ name: 'status', type: 'varchar', length: 50, nullable: true })
   status: string;
   ```

2. **DTO 파일** (API 응답에 포함되는 경우): `src/hanger/dto/hanger-response.dto.ts`
   ```typescript
   @ApiProperty({ description: '상태', example: 'active', nullable: true })
   status?: string;
   ```

3. **서비스 파일** (해당 필드를 사용하는 로직이 있는 경우): `src/hanger/hanger.service.ts`
   - 필터링, 정렬 등에 사용 시 수정 필요

#### 체크리스트
- [ ] 엔티티에 컬럼 추가
- [ ] DTO에 필드 추가 (API 응답에 포함되는 경우)
- [ ] Swagger 문서 자동 반영 확인
- [ ] 기존 데이터 마이그레이션 고려 (nullable이므로 불필요)

---

### 3.2 시나리오 2: 컬럼 추가 (not null)

**예시**: `clothes` 테이블에 `category` 컬럼 추가 (VARCHAR(100), NOT NULL)

#### 수정 파일 목록

1. **엔티티 파일**: `src/entities/clothes.entity.ts`
   ```typescript
   @Column({ name: 'category', type: 'varchar', length: 100, nullable: false })
   category: string;
   ```

2. **DTO 파일**: `src/clothes/clothes.controller.ts` (응답 DTO가 있는 경우)
   ```typescript
   @ApiProperty({ description: '카테고리', example: '상의' })
   category: string;
   ```

3. **서비스 파일**: `src/clothes/clothes.service.ts`
   - 조회 로직에 영향 없음 (기본 조회는 자동 포함)
   - 필터링/검색 로직 추가 시 수정 필요

4. **데이터 마이그레이션**
   - 기존 데이터에 기본값 설정 필요
   - SQL 마이그레이션 스크립트 작성 권장

#### 체크리스트
- [ ] 엔티티에 컬럼 추가 (nullable: false)
- [ ] DTO에 필드 추가
- [ ] 기존 데이터 마이그레이션 스크립트 작성
- [ ] 기본값 설정 전략 수립

---

### 3.3 시나리오 3: 컬럼 삭제

**예시**: `hanger` 테이블에서 `last_pickdown_uuid` 컬럼 삭제

#### 수정 파일 목록

1. **엔티티 파일**: `src/entities/hanger.entity.ts`
   - 해당 컬럼 정의 제거

2. **DTO 파일**: `src/hanger/dto/hanger-response.dto.ts`
   - 해당 필드 제거

3. **서비스 파일**: `src/hanger/hanger.service.ts`
   - 해당 필드를 참조하는 로직 제거

4. **컨트롤러 파일**: `src/hanger/hanger.controller.ts`
   - Swagger 문서 자동 업데이트 확인

#### 체크리스트
- [ ] 엔티티에서 컬럼 제거
- [ ] DTO에서 필드 제거
- [ ] 서비스에서 해당 필드 사용 로직 제거
- [ ] 데이터 백업 확인 (삭제 전)

---

### 3.4 시나리오 4: 컬럼 타입 변경

**예시**: `clothes.price` 컬럼 타입을 INT에서 DECIMAL(10,2)로 변경

#### 수정 파일 목록

1. **엔티티 파일**: `src/entities/clothes.entity.ts`
   ```typescript
   // 변경 전
   @Column({ name: 'price', type: 'int', nullable: true })
   price: number;

   // 변경 후
   @Column({ name: 'price', type: 'decimal', precision: 10, scale: 2, nullable: true })
   price: number; // 또는 string (정밀도 필요 시)
   ```

2. **DTO 파일**: `src/clothes/clothes.controller.ts`
   - 타입 일치 확인

3. **서비스 파일**: `src/clothes/clothes.service.ts`
   - 계산 로직이 있는 경우 타입 변환 고려

4. **데이터 마이그레이션**
   - 기존 데이터 변환 스크립트 작성

#### 체크리스트
- [ ] 엔티티 타입 변경
- [ ] DTO 타입 일치 확인
- [ ] 서비스 로직 타입 변환 검토
- [ ] 데이터 마이그레이션 스크립트 작성

---

### 3.5 시나리오 5: 외래키 관계 추가

**예시**: `hanger` 테이블에 `hmi_content_seq` FK 추가 (→ hmi_content.seq)

#### 수정 파일 목록

1. **엔티티 파일**: `src/entities/hanger.entity.ts`
   ```typescript
   // 컬럼 추가
   @Column({ name: 'hmi_content_seq', type: 'int', nullable: true })
   hmiContentSeq: number;

   // 관계 추가
   @ManyToOne(() => HmiContent, { nullable: true })
   @JoinColumn({ name: 'hmi_content_seq', referencedColumnName: 'seq' })
   hmiContent: HmiContent;
   ```

2. **엔티티 import 추가**
   ```typescript
   import { HmiContent } from './hmi-content.entity';
   ```

3. **DTO 파일**: `src/hanger/dto/hanger-response.dto.ts`
   ```typescript
   @ApiProperty({ description: 'HMI 콘텐츠 정보', type: () => HmiContent, nullable: true })
   hmiContent?: HmiContent;
   ```

4. **서비스 파일**: `src/hanger/hanger.service.ts`
   ```typescript
   // 관계 포함 조회
   const hangers = await this.hangerRepository.find({
     relations: ['clothes', 'hmiContent'], // 추가
   });
   ```

5. **모듈 파일**: `src/app.module.ts`
   - 엔티티가 이미 등록되어 있으므로 수정 불필요

#### 체크리스트
- [ ] 엔티티에 컬럼 추가
- [ ] 엔티티에 관계 데코레이터 추가
- [ ] DTO에 관계 필드 추가
- [ ] 서비스에서 relations 옵션 업데이트
- [ ] import 문 추가

---

### 3.6 시나리오 6: 새 테이블 추가

**예시**: `user` 테이블 추가

#### 수정 파일 목록

1. **엔티티 파일 생성**: `src/entities/user.entity.ts`
   ```typescript
   @Entity('user')
   export class User {
     @PrimaryColumn({ name: 'seq', type: 'int' })
     seq: number;

     @Column({ name: 'name', type: 'varchar', length: 100 })
     name: string;
     // ... 기타 컬럼
   }
   ```

2. **모듈 생성**: `src/user/user.module.ts`
   ```typescript
   @Module({
     imports: [TypeOrmModule.forFeature([User])],
     controllers: [UserController],
     providers: [UserService],
     exports: [UserService],
   })
   export class UserModule {}
   ```

3. **서비스 생성**: `src/user/user.service.ts`
   ```typescript
   @Injectable()
   export class UserService {
     constructor(
       @InjectRepository(User)
       private userRepository: Repository<User>,
     ) {}
     // ... 메서드 구현
   }
   ```

4. **컨트롤러 생성**: `src/user/user.controller.ts`
   ```typescript
   @ApiTags('user')
   @Controller('user')
   export class UserController {
     // ... 엔드포인트 구현
   }
   ```

5. **루트 모듈 등록**: `src/app.module.ts`
   ```typescript
   imports: [
     // ... 기존 imports
     UserModule, // 추가
   ],
   ```

6. **TypeORM 설정 업데이트**: `src/app.module.ts`
   ```typescript
   entities: [Hanger, Clothes, HangerLog, Hangerleg, HmiContent, User], // 추가
   ```

#### 체크리스트
- [ ] 엔티티 파일 생성
- [ ] 모듈 파일 생성
- [ ] 서비스 파일 생성
- [ ] 컨트롤러 파일 생성
- [ ] DTO 파일 생성 (필요 시)
- [ ] app.module.ts에 모듈 등록
- [ ] app.module.ts에 엔티티 등록

---

### 3.7 시나리오 7: 테이블 삭제

**예시**: `hmi_content` 테이블 삭제

#### 수정 파일 목록

1. **엔티티 파일 삭제**: `src/entities/hmi-content.entity.ts`
   - 파일 삭제 또는 주석 처리

2. **모듈 파일 삭제**: `src/hmi-content/` 디렉터리 전체
   - 또는 모듈 비활성화

3. **루트 모듈 수정**: `src/app.module.ts`
   ```typescript
   imports: [
     // HmiContentModule, // 제거
   ],
   ```

4. **TypeORM 설정 수정**: `src/app.module.ts`
   ```typescript
   entities: [Hanger, Clothes, HangerLog, Hangerleg], // HmiContent 제거
   ```

5. **다른 엔티티에서 참조하는 경우**
   - FK 관계 제거
   - 참조하는 엔티티 수정

#### 체크리스트
- [ ] 엔티티 파일 삭제/비활성화
- [ ] 모듈 디렉터리 삭제/비활성화
- [ ] app.module.ts에서 모듈 제거
- [ ] app.module.ts에서 엔티티 제거
- [ ] 다른 엔티티의 참조 제거
- [ ] 데이터 백업 확인

---

## 4. 파일별 수정 체크리스트

### 4.1 엔티티 파일 (`src/entities/*.entity.ts`)

#### 컬럼 관련
- [ ] `@Column` 데코레이터 추가/수정/삭제
- [ ] 컬럼명 매핑 확인 (`name` 속성)
- [ ] 타입 매핑 확인 (`type` 속성)
- [ ] `nullable` 옵션 확인
- [ ] `length`, `precision`, `scale` 등 옵션 확인

#### 관계 관련
- [ ] `@ManyToOne`, `@OneToMany`, `@OneToOne`, `@ManyToMany` 데코레이터
- [ ] `@JoinColumn` 데코레이터의 `name` 속성 (FK 컬럼명)
- [ ] `@JoinColumn` 데코레이터의 `referencedColumnName` 속성 (참조 컬럼명)
- [ ] `nullable` 옵션 확인

#### Import 관련
- [ ] 관련 엔티티 import 추가/제거

---

### 4.2 DTO 파일 (`src/[domain]/dto/*.dto.ts`)

- [ ] 필드 추가/수정/삭제
- [ ] `@ApiProperty` 데코레이터 추가/수정
- [ ] 타입 일치 확인 (엔티티와 DTO)
- [ ] `nullable` 옵션 일치 확인

---

### 4.3 서비스 파일 (`src/[domain]/[domain].service.ts`)

#### Repository 사용
- [ ] `find()` 메서드의 `relations` 옵션 업데이트
- [ ] `findOne()` 메서드의 `relations` 옵션 업데이트
- [ ] `where` 조건에 변경된 컬럼 사용 시 수정

#### 비즈니스 로직
- [ ] 변경된 컬럼을 사용하는 로직 수정
- [ ] 타입 변환 로직 추가 (타입 변경 시)
- [ ] 필터링/정렬 로직 수정

---

### 4.4 컨트롤러 파일 (`src/[domain]/[domain].controller.ts`)

- [ ] 파라미터 변경 (컬럼명 변경 시)
- [ ] Swagger 문서 자동 반영 확인
- [ ] 응답 타입 확인 (DTO 변경 시)

---

### 4.5 모듈 파일 (`src/[domain]/[domain].module.ts`)

- [ ] `TypeOrmModule.forFeature`에 새 엔티티 추가
- [ ] 기존 엔티티 제거 (테이블 삭제 시)

---

### 4.6 루트 모듈 파일 (`src/app.module.ts`)

- [ ] `imports` 배열에 새 모듈 추가/제거
- [ ] `TypeOrmModule.forRootAsync`의 `entities` 배열 업데이트
- [ ] 엔티티 import 추가/제거

---

## 5. 데이터 마이그레이션 가이드

### 5.1 마이그레이션 전략

현재 프로젝트는 `synchronize: false`로 설정되어 있으므로, 수동 마이그레이션 필요.

#### 권장 접근법

1. **SQL 마이그레이션 스크립트 작성**
   ```sql
   -- 예: 컬럼 추가
   ALTER TABLE hanger ADD COLUMN status VARCHAR(50) NULL;
   ```

2. **데이터 변환 스크립트 작성** (필요 시)
   ```sql
   -- 예: 기존 데이터에 기본값 설정
   UPDATE hanger SET status = 'active' WHERE status IS NULL;
   ```

3. **롤백 스크립트 작성** (권장)
   ```sql
   -- 예: 롤백
   ALTER TABLE hanger DROP COLUMN status;
   ```

### 5.2 마이그레이션 체크리스트

- [ ] SQL 마이그레이션 스크립트 작성
- [ ] 데이터 변환 스크립트 작성 (필요 시)
- [ ] 롤백 스크립트 작성
- [ ] 테스트 환경에서 먼저 실행
- [ ] 백업 확인
- [ ] 프로덕션 적용 계획 수립

---

## 6. 테스트 체크리스트

스키마 변경 후 다음 테스트 수행:

### 6.1 단위 테스트
- [ ] 엔티티 로드 테스트
- [ ] Repository 조회 테스트
- [ ] 관계 조회 테스트 (FK 추가/변경 시)

### 6.2 통합 테스트
- [ ] API 엔드포인트 테스트
- [ ] 응답 데이터 구조 확인
- [ ] Swagger 문서 확인

### 6.3 수동 테스트
- [ ] 데이터베이스 연결 확인
- [ ] 쿼리 실행 확인
- [ ] 관계 조인 확인

---

## 7. 변경 영향도 평가

### 7.1 영향도 평가 기준

**낮음**: 기존 기능에 영향 없음
- nullable 컬럼 추가
- 새 테이블 추가 (독립적)

**중간**: 일부 기능 수정 필요
- not null 컬럼 추가
- 컬럼 타입 변경
- FK 추가

**높음**: 다수 파일 수정 필요
- 컬럼 삭제
- FK 삭제
- 컬럼명 변경

**매우 높음**: 전체 구조 변경 필요
- PK 변경
- 테이블 삭제
- 주요 관계 변경

### 7.2 변경 전 확인 사항

1. **의존성 확인**
   - 다른 테이블에서 FK로 참조하는지
   - 다른 서비스에서 사용하는지

2. **데이터 확인**
   - 기존 데이터 보존 필요 여부
   - 데이터 변환 필요 여부

3. **API 호환성**
   - 기존 API 응답 구조 변경 여부
   - 클라이언트 영향도

---

## 8. 자동화 가능 영역

### 8.1 현재 수동 작업 영역

- 엔티티 파일 수정
- DTO 파일 수정
- 서비스 로직 수정
- 모듈 등록

### 8.2 향후 자동화 고려 사항

- **TypeORM Migration**: 마이그레이션 파일 자동 생성
- **엔티티 생성기**: SQL 스키마에서 엔티티 자동 생성
- **DTO 생성기**: 엔티티에서 DTO 자동 생성

---

## 9. 참고 자료

### 9.1 TypeORM 문서
- [엔티티 데코레이터](https://typeorm.io/entities)
- [관계 데코레이터](https://typeorm.io/relations)
- [마이그레이션](https://typeorm.io/migrations)

### 9.2 NestJS 문서
- [TypeORM 통합](https://docs.nestjs.com/techniques/database)
- [모듈 구조](https://docs.nestjs.com/modules)

---

## 10. 변경 이력 관리

스키마 변경 시 다음 정보를 문서화:

1. **변경 일자**
2. **변경 내용** (간단 요약)
3. **영향 파일 목록**
4. **마이그레이션 스크립트 경로**
5. **테스트 결과**
6. **롤백 계획**

---

## 부록: 빠른 참조 테이블

### A. 컬럼 타입 매핑

| MySQL 타입 | TypeORM 타입 | TypeScript 타입 |
|-----------|-------------|----------------|
| INT | 'int' | number |
| VARCHAR(n) | 'varchar', length: n | string |
| TEXT | 'text' | string |
| DATETIME | 'datetime' | Date |
| TINYINT | 'tinyint' | number |
| JSON | 'json' | object |
| DECIMAL(p,s) | 'decimal', precision: p, scale: s | number |

### B. 관계 데코레이터 매핑

| 관계 유형 | TypeORM 데코레이터 | 예시 |
|---------|------------------|------|
| 다대일 | @ManyToOne | Hanger → Clothes |
| 일대다 | @OneToMany | Clothes → Hanger[] |
| 일대일 | @OneToOne | User → Profile |
| 다대다 | @ManyToMany | User ↔ Role |

---

**문서 버전**: 1.0  
**최종 업데이트**: 2025-01-27  
**작성자**: AI Assistant (Co-Lead)
