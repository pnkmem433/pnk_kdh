# 스마트 헹거/헹거랙 교체 기능 구현 체크리스트

## 개요

스마트 헹거 교체 및 스마트 헹거랙 교체 기능을 구현하기 위한 단계별 작업 리스트입니다.

---

## 데이터 모델 구조

### 개념 정리

- **`rack` (붙을 행거랙)**: 물리적인 행거랙 구조
  - 예: "1층 매장 A구역 행거랙 1번"
  - `rack_number`: 행거랙 식별 번호
  - `rack_location`: 행거랙 위치 정보

- **`hangerleg` (스마트 행거랙)**: IoT 디바이스로, 특정 `rack`에 설치됨
  - 예: "1층 매장 A구역 행거랙 1번의 3번째 위치에 설치된 스마트 행거랙"
  - `rack_seq`: 어느 붙을 행거랙에 속하는지
  - `position`: 해당 행거랙 내에서의 순서 (1, 2, 3, 4, 5...)

- **`hanger` (스마트 헹거)**: 의류를 걸 수 있는 스마트 헹거
  - `hangerleg_seq`: 어느 스마트 행거랙에 연결되어 있는지

### 데이터베이스 관계도

```
rack (붙을 행거랙)
├── seq (PK)
├── rack_number
├── rack_location
└── created_at

hangerleg (스마트 행거랙)
├── seq (PK)
├── uuid
├── rack_seq (FK → rack.seq)  ← 새로 추가
├── position (INT)             ← 새로 추가
└── (기존 hangerleg들이 rack에 속하도록 설정)

hanger (스마트 헹거)
├── seq (PK)
├── uuid
├── clothes_seq (FK → clothes.seq)
├── hangerleg_seq (FK → hangerleg.seq)
├── nfc_active
└── last_pickdown_uuid
```

### 관계 요약

- **Rack → Hangerleg**: OneToMany (하나의 붙을 행거랙에 여러 스마트 행거랙 설치 가능)
- **Hangerleg → Rack**: ManyToOne (스마트 행거랙은 하나의 붙을 행거랙에 속함)
- **Hangerleg → Hanger**: OneToMany (하나의 스마트 행거랙에 여러 스마트 헹거 연결 가능)
- **Hanger → Hangerleg**: ManyToOne (스마트 헹거는 하나의 스마트 행거랙에 연결됨)

---

## 1. 데이터베이스 스키마 변경

### 1.1 rack 테이블 생성 (붙을 행거랙)
- [ ] `rack` 테이블 생성:
  - `seq` (INT, PK, AUTO_INCREMENT) - 붙을 행거랙 시퀀스 ID
  - `rack_number` (VARCHAR 또는 INT) - 헹거랙 넘버/식별자
  - `rack_location` (VARCHAR 또는 TEXT) - 헹거랙 위치 정보
  - `created_at` (DATETIME) - 생성일시 (선택사항)
- [ ] 컬럼 타입 결정:
  - `rack_number`: VARCHAR(50) 또는 INT (비즈니스 요구사항 확인 필요)
  - `rack_location`: VARCHAR(255) 또는 TEXT (위치 정보 길이에 따라 결정)
- [ ] 인덱스 추가 고려 (`rack_number`에 UNIQUE 인덱스 등)
- [ ] 마이그레이션 SQL 스크립트 작성

**예상 SQL:**
```sql
CREATE TABLE IF NOT EXISTS `rack` (
  `seq` int NOT NULL AUTO_INCREMENT,
  `rack_number` VARCHAR(50) NOT NULL,
  `rack_location` VARCHAR(255) NULL,
  `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`seq`),
  UNIQUE KEY `rack_number` (`rack_number`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```

### 1.2 hangerleg 테이블 컬럼 추가
- [ ] `hangerleg` 테이블에 다음 컬럼 추가:
  - `rack_seq` (INT, FK → rack.seq) - 어느 붙을 행거랙에 있는지
  - `position` (INT) - 순서 (1, 2, 3, 4, 5...)
- [ ] 컬럼 타입 결정:
  - `rack_seq`: INT, NULL 허용 여부 결정 (초기 데이터 마이그레이션 고려)
  - `position`: INT, NULL 허용 여부 결정
- [ ] 외래키 제약조건 추가
- [ ] 인덱스 추가 고려 (`rack_seq`, `position` 조합 인덱스 등)
- [ ] 마이그레이션 SQL 스크립트 작성

**예상 SQL:**
```sql
ALTER TABLE `hangerleg` 
ADD COLUMN `rack_seq` INT NULL AFTER `uuid`,
ADD COLUMN `position` INT NULL AFTER `rack_seq`,
ADD CONSTRAINT `FK_hangerleg_rack` FOREIGN KEY (`rack_seq`) REFERENCES `rack` (`seq`),
ADD INDEX `idx_rack_position` (`rack_seq`, `position`);
```

---

## 2. 엔티티 레이어 업데이트

### 2.1 Rack 엔티티 생성
- [ ] `src/entities/rack.entity.ts` 파일 생성
- [ ] 필드 정의:
  - `seq` (INT, PK)
  - `rackNumber` (VARCHAR) - 헹거랙 넘버
  - `rackLocation` (VARCHAR) - 헹거랙 위치
  - `createdAt` (DATETIME) - 생성일시 (선택사항)
- [ ] `@ApiProperty` 데코레이터 추가 (Swagger 문서화)
- [ ] 관계 설정: `@OneToMany(() => Hangerleg)` 추가 (hangerlegs 관계)

**예상 엔티티:**
```typescript
@Entity('rack')
export class Rack {
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @Column({ name: 'rack_number', type: 'varchar', length: 50 })
  rackNumber: string;

  @Column({ name: 'rack_location', type: 'varchar', length: 255, nullable: true })
  rackLocation: string;

  @OneToMany(() => Hangerleg, (hangerleg) => hangerleg.rack)
  hangerlegs: Hangerleg[];
}
```

### 2.2 Hangerleg 엔티티 수정
- [ ] `src/entities/hangerleg.entity.ts` 파일 수정
- [ ] `rackSeq` 필드 추가 (컬럼명: `rack_seq`, FK → rack.seq)
- [ ] `position` 필드 추가 (컬럼명: `position`, INT)
- [ ] `@ManyToOne(() => Rack)` 관계 추가
- [ ] `@JoinColumn` 데코레이터 추가
- [ ] `@ApiProperty` 데코레이터 추가 (Swagger 문서화)
- [ ] 타입 및 nullable 옵션 설정

**예상 변경사항:**
```typescript
@Column({ name: 'rack_seq', type: 'int', nullable: true })
rackSeq: number;

@Column({ name: 'position', type: 'int', nullable: true })
position: number;

@ManyToOne(() => Rack, { nullable: true })
@JoinColumn({ name: 'rack_seq', referencedColumnName: 'seq' })
rack: Rack;
```

---

## 3. DTO 레이어 생성/수정

### 3.1 스마트 헹거 교체 요청 DTO 생성
- [ ] `src/hanger/dto/replace-hanger.dto.ts` 파일 생성
- [ ] 요청 필드 정의:
  - `hangerUuid` (string) - 스마트 헹거 UUID
  - `hangerlegUuid` (string) - 스마트 헹거랙 UUID
- [ ] `class-validator` 데코레이터 추가 (`@IsString()`, `@IsNotEmpty()` 등)
- [ ] `@ApiProperty` 데코레이터 추가

### 3.2 스마트 헹거랙 교체 요청 DTO 생성
- [ ] `src/hangerleg/dto/replace-hangerleg.dto.ts` 파일 생성
- [ ] 요청 필드 정의:
  - `hangerlegUuid` (string) - 스마트 헹거랙 UUID
  - `rackUuid` 또는 `rackSeq` (string 또는 number) - 붙을 행거랙 UUID 또는 SEQ
  - `position` (number) - 순서 (1, 2, 3, 4, 5...)
- [ ] `class-validator` 데코레이터 추가 (`@IsUUID()`, `@IsInt()`, `@Min(1)` 등)
- [ ] `@ApiProperty` 데코레이터 추가

**참고**: `rackUuid`를 사용할 경우 서비스에서 UUID → SEQ 변환 필요

### 3.3 응답 DTO (선택사항)
- [ ] 교체 성공/실패 응답 DTO 생성 (필요시)
- [ ] 또는 기존 엔티티를 응답으로 사용

---

## 4. 서비스 레이어 구현

### 4.1 HangerService에 교체 메서드 추가
- [ ] `src/hanger/hanger.service.ts` 파일 수정
- [ ] `replaceHanger()` 메서드 구현:
  - 입력: `hangerUuid`, `hangerlegUuid`
  - 로직:
    1. `hangerUuid`로 `hanger` 테이블에서 헹거 조회
    2. `hangerlegUuid`로 `hangerleg` 테이블에서 헹거랙 조회하여 `seq` 획득
    3. `hanger.hangerleg_seq`를 헹거랙의 `seq`로 업데이트
    4. 업데이트된 헹거 반환 또는 성공 여부 반환
  - 에러 처리: 헹거 또는 헹거랙이 존재하지 않을 경우 예외 처리
- [ ] `HangerlegService` 의존성 주입 (또는 직접 Repository 사용)

### 4.2 HangerlegService에 교체 메서드 추가
- [ ] `src/hangerleg/hangerleg.service.ts` 파일 수정
- [ ] `replaceHangerleg()` 메서드 구현:
  - 입력: `hangerlegUuid`, `rackUuid` 또는 `rackSeq`, `position`
  - 로직:
    1. `hangerlegUuid`로 `hangerleg` 테이블에서 스마트 헹거랙 조회
    2. `rackUuid`가 제공된 경우 `rack` 테이블에서 `rackSeq` 획득 (또는 직접 `rackSeq` 사용)
    3. `hangerleg.rack_seq`와 `hangerleg.position` 필드 업데이트
    4. 업데이트된 헹거랙 반환 또는 성공 여부 반환
  - 에러 처리: 헹거랙 또는 붙을 행거랙이 존재하지 않을 경우 예외 처리
- [ ] `RackService` 또는 `RackRepository` 의존성 주입 (UUID → SEQ 변환 시)

---

## 5. 컨트롤러 레이어 구현

### 5.1 HangerController에 교체 엔드포인트 추가
- [ ] `src/hanger/hanger.controller.ts` 파일 수정
- [ ] `PATCH /hanger/replace` 또는 `PUT /hanger/replace` 엔드포인트 추가
- [ ] `@Body()` 데코레이터로 요청 DTO 받기
- [ ] `@ApiOperation` 데코레이터 추가 (Swagger 문서화)
- [ ] `@ApiResponse` 데코레이터 추가 (성공/실패 응답 정의)
- [ ] 서비스 메서드 호출 및 응답 반환

**예상 엔드포인트:**
```typescript
@Patch('replace')
@ApiOperation({ summary: '스마트 헹거 교체', description: '스마트 헹거의 위치(헹거랙)를 변경합니다.' })
async replaceHanger(@Body() dto: ReplaceHangerDto) {
  return await this.hangerService.replaceHanger(dto);
}
```

### 5.2 HangerlegController에 교체 엔드포인트 추가
- [ ] `src/hangerleg/hangerleg.controller.ts` 파일 수정
- [ ] `PATCH /hangerleg/replace` 또는 `PUT /hangerleg/replace` 엔드포인트 추가
- [ ] `@Body()` 데코레이터로 요청 DTO 받기
- [ ] `@ApiOperation` 데코레이터 추가
- [ ] `@ApiResponse` 데코레이터 추가
- [ ] 서비스 메서드 호출 및 응답 반환

---

## 6. 모듈 레이어 생성/확인

### 6.1 Rack 모듈 생성 (새로 추가)
- [ ] `src/rack/rack.module.ts` 파일 생성
- [ ] `src/rack/rack.service.ts` 파일 생성
  - `findAll()`: 모든 붙을 행거랙 조회
  - `findOne(seq: number)`: 특정 붙을 행거랙 조회
  - `findByUuid(uuid: string)`: UUID로 조회 (필요시)
  - `findByRackNumber(rackNumber: string)`: 행거랙 번호로 조회 (필요시)
- [ ] `src/rack/rack.controller.ts` 파일 생성
  - `GET /rack`: 모든 붙을 행거랙 조회
  - `GET /rack/:seq`: 특정 붙을 행거랙 조회
- [ ] `TypeOrmModule.forFeature([Rack])` 추가
- [ ] `app.module.ts`에 `RackModule` import

### 6.2 모듈 의존성 확인
- [ ] `RackModule` 생성 (또는 기존 모듈에 통합)
  - `RackController`, `RackService`, `RackModule` 생성
  - `TypeOrmModule.forFeature([Rack])` 추가
- [ ] `HangerModule`에서 `HangerlegService` 사용 시 `HangerlegModule` import 확인
- [ ] `HangerlegModule`에서 `RackService` 사용 시 `RackModule` import 확인
- [ ] 또는 `HangerlegService`에서 `RackRepository` 직접 주입 시 `TypeOrmModule.forFeature([Rack])` 추가 확인
- [ ] `app.module.ts`에서 모든 엔티티가 등록되어 있는지 확인 (`Rack` 엔티티 추가)

---

## 7. 에러 처리 및 검증

### 7.1 입력 검증
- [ ] UUID 형식 검증 (`class-validator`의 `@IsUUID()` 사용 고려)
- [ ] 필수 필드 검증 (`@IsNotEmpty()` 등)
- [ ] DTO에 `ValidationPipe` 적용 확인 (`main.ts`에서 전역 설정 또는 컨트롤러 레벨)

### 7.2 비즈니스 로직 검증
- [ ] 헹거가 존재하는지 확인
- [ ] 스마트 헹거랙(hangerleg)이 존재하는지 확인
- [ ] 붙을 행거랙(rack)이 존재하는지 확인
- [ ] `position` 값이 유효한지 확인 (양수, 중복 체크 등)
- [ ] 존재하지 않을 경우 적절한 HTTP 상태 코드 반환 (404 Not Found)
- [ ] 업데이트 실패 시 예외 처리 (500 Internal Server Error)

---

## 8. 테스트 (선택사항)

### 8.1 단위 테스트
- [ ] 서비스 메서드 단위 테스트 작성
- [ ] Mock Repository 사용하여 테스트

### 8.2 통합 테스트
- [ ] API 엔드포인트 통합 테스트 작성
- [ ] 실제 데이터베이스 연결 테스트

### 8.3 수동 테스트
- [ ] Swagger UI를 통한 API 테스트
- [ ] Postman 또는 curl을 통한 테스트

---

## 9. 문서화

### 9.1 Swagger 문서
- [ ] 모든 엔드포인트에 대한 Swagger 문서 자동 생성 확인
- [ ] 요청/응답 예시 확인

### 9.2 코드 주석
- [ ] 복잡한 비즈니스 로직에 주석 추가
- [ ] 메서드 JSDoc 주석 추가 (선택사항)

---

## 10. 배포 전 확인사항

- [ ] 데이터베이스 마이그레이션 스크립트 실행 확인
  - `rack` 테이블 생성 확인
  - `hangerleg` 테이블 컬럼 추가 확인
  - 외래키 제약조건 확인
- [ ] 기존 데이터에 대한 마이그레이션 전략 수립
  - 기존 `hangerleg` 데이터에 대한 `rack_seq`, `position` 초기값 설정
  - 또는 NULL 허용 후 수동 입력
- [ ] API 엔드포인트 경로 확인
  - `/hanger/replace` (스마트 헹거 교체)
  - `/hangerleg/replace` (스마트 헹거랙 교체)
  - `/rack` (붙을 행거랙 조회, 선택사항)
- [ ] HTTP 메서드 확인 (PATCH vs PUT)
- [ ] CORS 설정 확인 (프론트엔드 연동 시)
- [ ] 환경 변수 확인 (데이터베이스 연결 등)
- [ ] 엔티티 관계 확인 (`Rack`, `Hangerleg`, `Hanger` 간 관계)
- [ ] 인덱스 성능 확인 (`rack_seq`, `position` 조합 인덱스)

---

## 구현 순서 권장사항

1. **데이터베이스 스키마 변경** (1단계)
   - `rack` 테이블 생성
   - `hangerleg` 테이블에 컬럼 추가
   - 초기 데이터 마이그레이션 (기존 hangerleg 데이터 처리)

2. **엔티티 업데이트** (2단계)
   - `Rack` 엔티티 생성
   - `Hangerleg` 엔티티 수정 (관계 추가)

3. **모듈 생성** (2.5단계)
   - `RackModule`, `RackService`, `RackController` 생성 (최소한 조회 기능)
   - 모듈 의존성 설정

4. **DTO 생성** (3단계)
   - 교체 요청 DTO 생성
   - 검증 로직 추가

5. **서비스 메서드 구현** (4단계)
   - `HangerService.replaceHanger()` 구현
   - `HangerlegService.replaceHangerleg()` 구현
   - UUID → SEQ 변환 로직 구현

6. **컨트롤러 엔드포인트 추가** (5단계)
   - 교체 API 엔드포인트 추가
   - Swagger 문서화

7. **에러 처리 및 검증** (6-7단계)
   - 입력 검증 강화
   - 비즈니스 로직 검증 (존재 여부, 중복 체크 등)

8. **테스트 및 문서화** (8-9단계)
   - 단위 테스트 작성
   - 통합 테스트 작성
   - API 문서 확인

---

## 주의사항

1. **UUID vs SEQ**: 
   - 요청은 UUID로 받지만, 데이터베이스 업데이트는 `seq`를 사용해야 함
   - UUID → SEQ 변환 로직 필요
   - `rack` 테이블에도 UUID가 필요한지 확인 필요 (현재는 `rack_number`만 있음)

2. **데이터 모델 구조**:
   - `rack` (붙을 행거랙): 물리적인 행거랙 구조
   - `hangerleg` (스마트 행거랙): IoT 디바이스, `rack`에 속하고 `position`으로 순서 표시
   - `hanger` (스마트 헹거): `hangerleg`에 연결됨

3. **트랜잭션 처리**:
   - 여러 테이블을 업데이트하는 경우 트랜잭션 고려 필요
   - `hangerleg` 교체 시 `rack` 조회 후 업데이트하므로 트랜잭션 고려

4. **기존 데이터 호환성**:
   - `rack` 테이블은 새로 생성되므로 초기 데이터 마이그레이션 필요
   - `hangerleg` 테이블에 새 컬럼 추가 시 기존 데이터는 NULL이 될 수 있음
   - NULL 허용 여부 결정 필요

5. **Position 중복 체크**:
   - 같은 `rack_seq` 내에서 `position`이 중복되지 않도록 검증 필요
   - 비즈니스 로직에서 중복 체크 또는 데이터베이스 제약조건 고려

6. **API 설계**:
   - RESTful 원칙에 따라 `PATCH` 또는 `PUT` 사용
   - 교체(Replace) 동작이므로 `PATCH`가 더 적합할 수 있음

7. **Rack 관리 API**:
   - `rack` 테이블 관리를 위한 CRUD API도 필요할 수 있음 (선택사항)
   - 최소한 조회 API는 필요할 것으로 예상
