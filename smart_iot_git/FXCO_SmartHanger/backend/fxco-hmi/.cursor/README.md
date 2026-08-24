# FXCO-HMI 백엔드 프로젝트 Cursor Rules 가이드

이 디렉터리는 FXCO-HMI 백엔드 프로젝트의 개발 가이드와 규칙을 담고 있습니다.

---

## 📚 문서 목록

### 1. [rules.md](./rules.md)
**프로젝트 개발 규칙 및 가이드라인**

이 문서는 프로젝트의 전반적인 개발 규칙을 정의합니다:
- 프로젝트 구조 규칙
- 네이밍 규칙
- 데이터베이스 및 엔티티 규칙
- API 설계 규칙
- 모듈 구조 규칙
- 변경 관리 규칙

**언제 사용하나요?**
- 새 기능 개발 시
- 코드 리뷰 시
- 프로젝트 구조 이해 시

---

### 2. [database-change-analysis.md](./database-change-analysis.md)
**데이터베이스 스키마 변경 대응 상세 가이드**

이 문서는 ERD/SQL 변경 시 어떤 파일을 어떻게 수정해야 하는지 상세히 설명합니다:
- 변경 유형별 시나리오 가이드
- 파일별 수정 체크리스트
- 데이터 마이그레이션 가이드
- 테스트 체크리스트

**언제 사용하나요?**
- 데이터베이스 스키마 변경 시
- 테이블/컬럼 추가/수정/삭제 시
- 외래키 관계 변경 시

**주요 시나리오**:
- ✅ 컬럼 추가 (nullable/not null)
- ✅ 컬럼 삭제
- ✅ 컬럼 타입 변경
- ✅ 외래키 관계 추가
- ✅ 새 테이블 추가
- ✅ 테이블 삭제

---

### 3. [project-structure-analysis.md](./project-structure-analysis.md)
**프로젝트 구조 분석 및 변경 가능 영역 리스트업**

이 문서는 프로젝트의 현재 구조를 분석하고 개선점을 제시합니다:
- 아키텍처 개요
- 변경 가능 영역 분석
- 개선 제안 사항
- 변경 영향도 매트릭스

**언제 사용하나요?**
- 프로젝트 구조 이해 시
- 리팩토링 계획 수립 시
- 개선점 도출 시

---

## 🚀 빠른 시작

### 데이터베이스 스키마 변경 시

1. **변경 사항 확인**
   - ERD 또는 SQL 스크립트 확인
   - 변경 유형 파악 (컬럼 추가/삭제/수정 등)

2. **가이드 참조**
   - `database-change-analysis.md`에서 해당 시나리오 찾기
   - 체크리스트 따라 수정

3. **파일 수정**
   - 엔티티 파일 (`src/entities/*.entity.ts`)
   - DTO 파일 (`src/[domain]/dto/*.dto.ts`)
   - 서비스 파일 (`src/[domain]/[domain].service.ts`)
   - 모듈 파일 (`src/[domain]/[domain].module.ts`)

4. **테스트**
   - 단위 테스트 실행
   - 통합 테스트 실행
   - 수동 테스트 실행

---

## 📋 주요 체크리스트

### 새 기능 추가 시
- [ ] 엔티티 확인/생성
- [ ] 모듈 생성
- [ ] 서비스 생성
- [ ] 컨트롤러 생성
- [ ] DTO 생성
- [ ] `app.module.ts`에 모듈 등록
- [ ] Swagger 문서화

### 데이터베이스 변경 시
- [ ] 엔티티 파일 업데이트
- [ ] DTO 업데이트 (필요 시)
- [ ] 서비스 로직 검토
- [ ] 모듈 등록 확인
- [ ] 테스트 실행
- [ ] Change Report 작성

---

## 🔍 주요 개념

### 엔티티 (Entity)
데이터베이스 테이블을 TypeScript 클래스로 매핑한 것
- 위치: `src/entities/*.entity.ts`
- 역할: 데이터베이스 스키마 정의

### DTO (Data Transfer Object)
API 요청/응답 데이터 구조
- 위치: `src/[domain]/dto/*.dto.ts`
- 역할: API 인터페이스 정의

### 서비스 (Service)
비즈니스 로직 처리
- 위치: `src/[domain]/[domain].service.ts`
- 역할: 데이터 조회/처리 로직

### 컨트롤러 (Controller)
HTTP 요청/응답 처리
- 위치: `src/[domain]/[domain].controller.ts`
- 역할: API 엔드포인트 정의

### 모듈 (Module)
도메인별 기능 그룹화
- 위치: `src/[domain]/[domain].module.ts`
- 역할: 의존성 관리

---

## 📊 현재 데이터베이스 스키마

### 테이블 목록
- `hanger` - 행거 정보
- `clothes` - 의류 정보
- `hangerleg` - 행거랙 정보
- `hanger_log` - 행거 로그 정보
- `hmi_content` - HMI 콘텐츠 정보

### 주요 관계
- `Hanger` → `Clothes` (ManyToOne)
- `HangerLog` → `Hanger` (ManyToOne)
- `HangerLog` → `Hangerleg` (ManyToOne)

---

## 🛠️ 기술 스택

- **Framework**: NestJS
- **Language**: TypeScript
- **ORM**: TypeORM
- **Database**: MySQL
- **API Documentation**: Swagger

---

## 📝 변경 이력

### 2025-01-27
- Cursor Rules 문서 생성
- 데이터베이스 변경 대응 가이드 작성
- 프로젝트 구조 분석 문서 작성

---

## 💡 팁

1. **스키마 변경 전 항상 백업**
2. **변경 사항은 단계적으로 적용**
3. **테스트 환경에서 먼저 검증**
4. **Change Report 작성으로 추적성 확보**

---

## 🔗 관련 링크

- [프로젝트 README](../README.md)
- [개선 사항 문서](../docs/251218-improvements.md)
- [NestJS 공식 문서](https://docs.nestjs.com/)
- [TypeORM 공식 문서](https://typeorm.io/)

---

**문서 버전**: 1.0  
**최종 업데이트**: 2025-01-27
