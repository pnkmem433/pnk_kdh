# FXCO-HMI 프로젝트 구조 분석 및 변경 가능 영역 리스트업

## 목적

이 문서는 FXCO-HMI 백엔드 프로젝트의 현재 구조를 분석하고, ERD/SQL 변경에 유연하게 대응할 수 있는 영역과 개선이 필요한 영역을 식별합니다.

---

## 1. 프로젝트 아키텍처 개요

### 1.1 계층 구조

```
┌─────────────────────────────────────┐
│      Controller Layer               │  ← HTTP 요청/응답 처리
│  (hanger.controller.ts 등)          │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Service Layer                  │  ← 비즈니스 로직
│  (hanger.service.ts 등)             │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Repository Layer               │  ← 데이터 접근 (TypeORM)
│  (TypeORM Repository)               │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Entity Layer                   │  ← 데이터베이스 스키마 매핑
│  (hanger.entity.ts 등)              │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Database (MySQL)                │
└─────────────────────────────────────┘
```

### 1.2 모듈 구조

```
AppModule (루트)
├── HangerModule
│   ├── HangerController
│   ├── HangerService
│   └── HangerRepository (TypeORM)
├── ClothesModule
│   ├── ClothesController
│   ├── ClothesService
│   └── ClothesRepository (TypeORM)
├── HangerlegModule
│   ├── HangerlegController
│   ├── HangerlegService
│   └── HangerlegRepository (TypeORM)
└── HmiContentModule
    ├── HmiContentController
    ├── HmiContentService
    └── HmiContentRepository (TypeORM)
```

---

## 2. 변경 가능 영역 분석

### 2.1 유연하게 대응 가능한 영역 ✅

#### 2.1.1 엔티티 레이어 (높은 유연성)

**위치**: `src/entities/*.entity.ts`

**특징**:
- TypeORM 데코레이터 기반으로 스키마 변경에 유연하게 대응 가능
- 컬럼 추가/수정/삭제가 비교적 쉬움
- 관계(Relation) 변경도 데코레이터 수정으로 가능

**변경 예시**:
```typescript
// 컬럼 추가: 단순히 데코레이터 추가
@Column({ name: 'new_column', type: 'varchar', nullable: true })
newColumn: string;

// 관계 추가: 데코레이터 추가 및 import
@ManyToOne(() => RelatedEntity)
@JoinColumn({ name: 'fk_column' })
relatedEntity: RelatedEntity;
```

**제약사항**:
- 컬럼명은 `name` 속성으로 매핑되어 있어 변경 가능
- 타입은 TypeORM 타입과 일치해야 함

---

#### 2.1.2 DTO 레이어 (중간 유연성)

**위치**: `src/[domain]/dto/*.dto.ts`

**특징**:
- 엔티티와 분리되어 있어 API 응답 구조를 독립적으로 변경 가능
- Swagger 문서 자동 생성으로 문서화 부담 적음

**변경 예시**:
```typescript
// 필드 추가: 단순히 속성 추가
@ApiProperty({ description: '새 필드' })
newField: string;

// 필드 제거: 속성 삭제
// oldField: string; // 제거
```

**제약사항**:
- 엔티티와 DTO 간 타입 일치 필요 (런타임 에러 방지)

---

#### 2.1.3 서비스 레이어 (조건부 유연성)

**위치**: `src/[domain]/[domain].service.ts`

**특징**:
- TypeORM Repository를 사용하여 쿼리 로직이 유연함
- `relations` 옵션으로 관계 조인 쉽게 변경 가능

**변경 예시**:
```typescript
// 관계 추가: relations 배열에 추가
const entities = await this.repository.find({
  relations: ['clothes', 'newRelation'], // 추가
});

// 필터링 추가: where 조건 추가
const entities = await this.repository.find({
  where: { newColumn: value },
});
```

**제약사항**:
- 하드코딩된 쿼리 로직이 있으면 수정 필요
- 복잡한 조인 로직은 수동 수정 필요

---

#### 2.1.4 모듈 구조 (높은 유연성)

**위치**: `src/[domain]/[domain].module.ts`

**특징**:
- 새 도메인 추가 시 모듈 단위로 확장 가능
- 모듈 간 의존성 관리 용이

**변경 예시**:
```typescript
// 새 엔티티 추가: TypeOrmModule.forFeature에 추가
@Module({
  imports: [TypeOrmModule.forFeature([Entity, NewEntity])],
  // ...
})
```

---

### 2.2 개선이 필요한 영역 ⚠️

#### 2.2.1 하드코딩된 관계 조회

**위치**: `src/hanger/hanger.service.ts`

**문제점**:
```typescript
// 하드코딩된 relations
const hangers = await this.hangerRepository.find({
  relations: ['clothes'], // 하드코딩
});
```

**개선 방안**:
- 관계를 동적으로 로드하는 옵션 추가
- 쿼리 파라미터로 관계 포함 여부 제어

**예시 개선 코드**:
```typescript
async findAll(includeRelations?: string[]): Promise<HangerResponseDto[]> {
  const relations = includeRelations || ['clothes'];
  const hangers = await this.hangerRepository.find({
    relations,
  });
  // ...
}
```

---

#### 2.2.2 하드코딩된 로그 조회 로직

**위치**: `src/hanger/hanger.service.ts`

**문제점**:
```typescript
// 하드코딩된 정렬 기준
const latestLog = await this.hangerLogRepository.findOne({
  where: { hangerSeq },
  order: { seq: 'DESC' }, // seq 기준 정렬 하드코딩
});
```

**개선 방안**:
- 정렬 기준을 파라미터화
- `created_at` 기준 정렬 옵션 추가

**예시 개선 코드**:
```typescript
private async getCurrentHangerlegSeq(
  hangerSeq: number,
  orderBy: 'seq' | 'created_at' = 'seq'
): Promise<number | null> {
  const latestLog = await this.hangerLogRepository.findOne({
    where: { hangerSeq },
    order: { [orderBy]: 'DESC' },
  });
  return latestLog?.hangerlegSeq ?? null;
}
```

---

#### 2.2.3 엔티티 등록 중복

**위치**: `src/app.module.ts`

**문제점**:
```typescript
// 엔티티를 두 곳에 등록
entities: [Hanger, Clothes, HangerLog, Hangerleg, HmiContent], // 여기
// 그리고 각 모듈의 TypeOrmModule.forFeature에도 등록
```

**개선 방안**:
- `autoLoadEntities: true` 옵션 활용 (이미 설정됨)
- 루트 모듈의 `entities` 배열은 제거 가능 (중복)

**현재 상태**:
- `autoLoadEntities: true`가 설정되어 있어 중복 등록이 있지만 동작에는 문제 없음
- 명시적 등록이 선호되는 경우 유지 가능

---

#### 2.2.4 타입 안정성 부족

**위치**: 전역 (`tsconfig.json`)

**문제점**:
```json
{
  "strictNullChecks": false,
  "noImplicitAny": false,
  // ...
}
```

**개선 방안**:
- 점진적으로 strict 모드 활성화
- 타입 안정성 향상으로 런타임 에러 감소

**주의사항**:
- 기존 코드와의 호환성 고려 필요
- 단계적 적용 권장

---

### 2.3 변경이 어려운 영역 (의도적 설계) 🔒

#### 2.3.1 TypeORM 설정

**위치**: `src/app.module.ts`

**특징**:
- `synchronize: false`로 고정 (프로덕션 안정성)
- 데이터베이스 연결 설정은 환경 변수로 관리

**변경 필요성**: 낮음 (의도적 설계)

---

#### 2.3.2 모듈 구조

**위치**: `src/[domain]/`

**특징**:
- 도메인별 모듈 분리는 유지보수성 향상
- 변경 불필요 (설계 원칙)

---

## 3. ERD/SQL 변경 대응 전략

### 3.1 변경 유형별 대응 전략

| 변경 유형 | 대응 난이도 | 수정 파일 수 | 예상 시간 |
|---------|-----------|------------|---------|
| 컬럼 추가 (nullable) | ⭐ 쉬움 | 1-2개 | 5분 |
| 컬럼 추가 (not null) | ⭐⭐ 보통 | 2-3개 | 15분 |
| 컬럼 삭제 | ⭐⭐ 보통 | 2-4개 | 20분 |
| 컬럼 타입 변경 | ⭐⭐ 보통 | 2-3개 | 15분 |
| FK 추가 | ⭐⭐⭐ 어려움 | 3-5개 | 30분 |
| 새 테이블 추가 | ⭐⭐⭐ 어려움 | 5-7개 | 1시간 |
| 테이블 삭제 | ⭐⭐⭐⭐ 매우 어려움 | 5-10개 | 2시간 |

---

### 3.2 자동화 가능한 작업

#### 3.2.1 엔티티 생성 자동화

**도구**: TypeORM CLI 또는 커스텀 스크립트

**예시**:
```bash
# SQL 스키마에서 엔티티 자동 생성 (향후)
typeorm entity:create -n User
```

**현재 상태**: 수동 생성

---

#### 3.2.2 마이그레이션 자동화

**도구**: TypeORM Migrations

**예시**:
```bash
# 마이그레이션 생성
typeorm migration:generate -n AddStatusColumn

# 마이그레이션 실행
typeorm migration:run
```

**현재 상태**: 수동 SQL 스크립트 사용

**개선 제안**:
- TypeORM Migrations 도입 검토
- 마이그레이션 파일로 버전 관리

---

### 3.3 수동 작업 필수 영역

1. **비즈니스 로직 수정**
   - 서비스 레이어의 커스텀 로직
   - 복잡한 쿼리 로직

2. **DTO 구조 변경**
   - API 응답 구조 설계
   - Swagger 문서화

3. **테스트 작성**
   - 단위 테스트
   - 통합 테스트

---

## 4. 개선 제안 사항

### 4.1 단기 개선 (즉시 적용 가능)

#### 4.1.1 관계 조회 파라미터화

**파일**: `src/hanger/hanger.service.ts`

**변경 내용**:
```typescript
async findAll(includeRelations: string[] = ['clothes']): Promise<HangerResponseDto[]> {
  const hangers = await this.hangerRepository.find({
    relations: includeRelations,
  });
  // ...
}
```

**이점**:
- 관계 추가 시 유연성 향상
- 필요에 따라 관계 선택적 로드 가능

---

#### 4.1.2 정렬 기준 파라미터화

**파일**: `src/hanger/hanger.service.ts`

**변경 내용**:
```typescript
private async getCurrentHangerlegSeq(
  hangerSeq: number,
  orderBy: 'seq' | 'created_at' = 'seq'
): Promise<number | null> {
  // ...
}
```

**이점**:
- 정렬 기준 변경 시 유연성 향상
- 시간 기준 정렬 옵션 제공

---

### 4.2 중기 개선 (점진적 적용)

#### 4.2.1 TypeORM Migrations 도입

**목적**: 데이터베이스 스키마 변경을 버전 관리

**작업**:
1. TypeORM Migrations 설정
2. 기존 스키마를 마이그레이션 파일로 변환
3. 향후 변경사항을 마이그레이션으로 관리

**이점**:
- 스키마 변경 이력 관리
- 롤백 용이
- 팀 협업 시 충돌 방지

---

#### 4.2.2 엔티티 생성 자동화

**목적**: SQL 스키마에서 엔티티 자동 생성

**작업**:
1. 커스텀 스크립트 작성
2. SQL 스키마 파싱
3. TypeORM 엔티티 파일 생성

**이점**:
- 수동 작업 감소
- 일관성 유지

---

### 4.3 장기 개선 (구조적 변경)

#### 4.3.1 Repository 패턴 도입

**목적**: 데이터 접근 로직 추상화

**작업**:
1. 커스텀 Repository 인터페이스 정의
2. TypeORM Repository 래핑
3. 서비스에서 인터페이스 사용

**이점**:
- 데이터베이스 변경 시 유연성 향상
- 테스트 용이성 향상

---

#### 4.3.2 Query Builder 유틸리티

**목적**: 복잡한 쿼리 로직 재사용

**작업**:
1. 공통 쿼리 빌더 유틸리티 생성
2. 필터링, 정렬, 페이징 등 공통 로직 추상화

**이점**:
- 코드 중복 감소
- 쿼리 로직 일관성 유지

---

## 5. 변경 영향도 매트릭스

### 5.1 파일별 변경 영향도

| 파일 | 변경 빈도 | 영향 범위 | 유지보수 난이도 |
|-----|---------|---------|---------------|
| `entities/*.entity.ts` | 높음 | 중간 | 쉬움 |
| `dto/*.dto.ts` | 중간 | 낮음 | 쉬움 |
| `service/*.service.ts` | 중간 | 높음 | 보통 |
| `controller/*.controller.ts` | 낮음 | 낮음 | 쉬움 |
| `module/*.module.ts` | 낮음 | 중간 | 쉬움 |
| `app.module.ts` | 낮음 | 높음 | 보통 |

---

### 5.2 도메인별 변경 영향도

| 도메인 | 엔티티 수 | 관계 수 | 변경 영향도 |
|-------|---------|--------|-----------|
| Hanger | 1 | 2 (Clothes, HangerLog) | 높음 |
| Clothes | 1 | 1 (Hanger) | 중간 |
| Hangerleg | 1 | 1 (HangerLog) | 중간 |
| HangerLog | 1 | 2 (Hanger, Hangerleg) | 높음 |
| HmiContent | 1 | 0 | 낮음 |

---

## 6. 체크리스트: 스키마 변경 시

### 6.1 변경 전

- [ ] 변경 사항 문서화 (ERD/SQL 스크립트)
- [ ] 영향 범위 분석
- [ ] 백업 계획 수립
- [ ] 롤백 계획 수립

### 6.2 변경 중

- [ ] 엔티티 파일 수정
- [ ] DTO 파일 수정 (필요 시)
- [ ] 서비스 로직 수정 (필요 시)
- [ ] 모듈 등록 (새 테이블인 경우)

### 6.3 변경 후

- [ ] 단위 테스트 실행
- [ ] 통합 테스트 실행
- [ ] Swagger 문서 확인
- [ ] 수동 테스트 실행
- [ ] 변경 이력 문서화

---

## 7. 참고 자료

### 7.1 프로젝트 내 문서

- `.cursor/rules.md` - 프로젝트 규칙
- `.cursor/database-change-analysis.md` - 상세 변경 가이드
- `README.md` - 프로젝트 개요
- `docs/251218-improvements.md` - 개선 사항 문서

### 7.2 외부 문서

- [TypeORM 공식 문서](https://typeorm.io/)
- [NestJS 공식 문서](https://docs.nestjs.com/)
- [MySQL 공식 문서](https://dev.mysql.com/doc/)

---

**문서 버전**: 1.0  
**최종 업데이트**: 2025-01-27  
**작성자**: AI Assistant (Co-Lead)
