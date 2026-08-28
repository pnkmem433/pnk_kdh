# FXCO_SmartHanger 프로젝트별 데이터베이스/테이블 문서

## backend/fxco-hmi
### 1) 데이터베이스 개요
- 데이터베이스: `MySQL` (`fxco_db` 기본값)
- 문자셋/콜레이션: `utf8mb4 / utf8mb4_0900_ai_ci` (rack DDL 기준), 기타 테이블 `미확인`
- 접근 방식: `NestJS + TypeORM`
- 주요 도메인: `행거: hanger, hangerleg, hanger_log` / `의류: clothes` / `HMI 콘텐츠: hmi_content` / `랙: rack`

### 2) 테이블 사용 현황 요약
| 테이블명 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|
| `hanger` | 사용 | 행거 상태/매칭(의류, 위치, NFC) 관리 | `backend/fxco-hmi/src/entities/hanger.entity.ts:5`, `backend/fxco-hmi/src/hanger/hanger.service.ts:24`, `backend/fxco-hmi/src/hanger/hanger.service.ts:89` |
| `hangerleg` | 사용 | 행거 위치(leg) 조회/교체, rack 위치 배치 관리 | `backend/fxco-hmi/src/entities/hangerleg.entity.ts:5`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:56`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:85`, `backend/fxco-hmi/docs/migration-add-rack-table.sql:17` |
| `hanger_log` | 사용 | 행거 현재 위치 계산용 최근 로그 참조 | `backend/fxco-hmi/src/entities/hanger-log.entity.ts:4`, `backend/fxco-hmi/src/hanger/hanger.service.ts:57` |
| `clothes` | 사용 | 의류 목록/단건 조회, 행거-의류 매칭 대상 | `backend/fxco-hmi/src/entities/clothes.entity.ts:4`, `backend/fxco-hmi/src/clothes/clothes.service.ts:14`, `backend/fxco-hmi/src/hanger/hanger.service.ts:119` |
| `hmi_content` | 사용 | HMI 콘텐츠 목록/단건 조회 | `backend/fxco-hmi/src/entities/hmi-content.entity.ts:4`, `backend/fxco-hmi/src/hmi-content/hmi-content.service.ts:14` |
| `rack` | 사용 | 랙 마스터 조회, hangerleg 배치 대상 | `backend/fxco-hmi/src/entities/rack.entity.ts:5`, `backend/fxco-hmi/src/rack/rack.service.ts:14`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:50`, `backend/fxco-hmi/docs/migration-add-rack-table.sql:7` |

### 3) 테이블 상세
#### `hanger`
- 역할: 행거 식별/의류 매칭/현재 위치/태깅 상태 관리
- 사용 여부: 사용
- 근거: `backend/fxco-hmi/src/entities/hanger.entity.ts:5`, `backend/fxco-hmi/src/hanger/hanger.service.ts:74`, `backend/fxco-hmi/src/hanger/hanger.service.ts:125`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, 조회 키 | `backend/fxco-hmi/src/entities/hanger.entity.ts:8`, `backend/fxco-hmi/src/hanger/hanger.service.ts:30`, `backend/fxco-hmi/src/hanger/hanger.service.ts:41` |
| `uuid` | `varchar(50)` | 사용 | 행거 식별자 기반 조회/교체 | `backend/fxco-hmi/src/entities/hanger.entity.ts:12`, `backend/fxco-hmi/src/hanger/hanger.service.ts:75`, `backend/fxco-hmi/src/hanger/hanger.service.ts:111` |
| `clothes_seq` | `int` | 사용 | 의류 매칭/해제 | `backend/fxco-hmi/src/entities/hanger.entity.ts:16`, `backend/fxco-hmi/src/hanger/hanger.service.ts:126`, `backend/fxco-hmi/src/hanger/hanger.service.ts:131` |
| `hangerleg_seq` | `int` | 사용 | 위치 교체 시 연결 leg 갱신/해제 | `backend/fxco-hmi/src/entities/hanger.entity.ts:20`, `backend/fxco-hmi/src/hanger/hanger.service.ts:90`, `backend/fxco-hmi/src/hanger/hanger.service.ts:95` |
| `nfc_active` | `tinyint` | 사용 | API 응답 DTO 포함(행거 상태) | `backend/fxco-hmi/src/entities/hanger.entity.ts:24`, `backend/fxco-hmi/src/hanger/dto/hanger-response.dto.ts:18`, `backend/fxco-hmi/src/hanger/hanger.service.ts:23` |
| `last_pickdown_uuid` | `varchar(255)` | 사용 | API 응답 DTO 포함 | `backend/fxco-hmi/src/entities/hanger.entity.ts:28`, `backend/fxco-hmi/src/hanger/dto/hanger-response.dto.ts:21`, `backend/fxco-hmi/src/hanger/hanger.service.ts:23` |

#### `hangerleg`
- 역할: 행거 위치(leg) 마스터, rack/position 배치 정보
- 사용 여부: 사용
- 근거: `backend/fxco-hmi/src/entities/hangerleg.entity.ts:5`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:56`, `backend/fxco-hmi/docs/migration-add-rack-table.sql:17`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, 단건 조회/참조 치환 기준 | `backend/fxco-hmi/src/entities/hangerleg.entity.ts:8`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:66`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:93` |
| `uuid` | `varchar` | 사용 | leg 식별자 조회/교체 입력 키 | `backend/fxco-hmi/src/entities/hangerleg.entity.ts:12`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:29`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:77` |
| `rack_seq` | `int` | 사용 | rack 배치 키, 위치 검색/갱신 | `backend/fxco-hmi/src/entities/hangerleg.entity.ts:16`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:58`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:85`, `backend/fxco-hmi/docs/migration-add-rack-table.sql:18` |
| `position` | `int` | 사용 | rack 내 위치 검색/갱신 | `backend/fxco-hmi/src/entities/hangerleg.entity.ts:20`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:59`, `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts:86`, `backend/fxco-hmi/docs/migration-add-rack-table.sql:19` |

#### `hanger_log`
- 역할: 행거 위치 변경 이력(최신 위치 계산 포함)
- 사용 여부: 사용
- 근거: `backend/fxco-hmi/src/entities/hanger-log.entity.ts:4`, `backend/fxco-hmi/src/hanger/hanger.service.ts:57`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | 최신 로그 정렬 기준 | `backend/fxco-hmi/src/entities/hanger-log.entity.ts:7`, `backend/fxco-hmi/src/hanger/hanger.service.ts:59` |
| `hanger_seq` | `int` | 사용 | 특정 행거 로그 필터 | `backend/fxco-hmi/src/entities/hanger-log.entity.ts:11`, `backend/fxco-hmi/src/hanger/hanger.service.ts:58` |
| `hangerleg_seq` | `int` | 사용 | 현재 위치(hangerlegSeqCurrent) 계산 | `backend/fxco-hmi/src/entities/hanger-log.entity.ts:15`, `backend/fxco-hmi/src/hanger/hanger.service.ts:62` |
| `created_at` | `datetime` | 판단불가 | 엔티티 정의는 있으나 서비스에서 컬럼 단위 참조 없음 | `backend/fxco-hmi/src/entities/hanger-log.entity.ts:19`, `backend/fxco-hmi/src/hanger/hanger.service.ts:57` |
| `self_written` | `varchar` | 판단불가 | 엔티티 정의는 있으나 서비스에서 컬럼 단위 참조 없음 | `backend/fxco-hmi/src/entities/hanger-log.entity.ts:23`, `backend/fxco-hmi/src/hanger/hanger.service.ts:57` |

#### `clothes`
- 역할: 의류 마스터
- 사용 여부: 사용
- 근거: `backend/fxco-hmi/src/entities/clothes.entity.ts:4`, `backend/fxco-hmi/src/clothes/clothes.service.ts:14`, `backend/fxco-hmi/src/clothes/clothes.controller.ts:14`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, 단건 조회/매칭 키 | `backend/fxco-hmi/src/entities/clothes.entity.ts:7`, `backend/fxco-hmi/src/clothes/clothes.service.ts:18`, `backend/fxco-hmi/src/hanger/hanger.service.ts:119` |
| `name` | `varchar` | 사용 | clothes 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/clothes.entity.ts:11`, `backend/fxco-hmi/src/clothes/clothes.controller.ts:14` |
| `tag` | `varchar` | 사용 | clothes 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/clothes.entity.ts:15`, `backend/fxco-hmi/src/clothes/clothes.controller.ts:14` |
| `media` | `int` | 사용 | clothes 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/clothes.entity.ts:19`, `backend/fxco-hmi/src/clothes/clothes.controller.ts:14` |
| `size_color_options` | `json` | 사용 | clothes 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/clothes.entity.ts:26`, `backend/fxco-hmi/src/clothes/clothes.controller.ts:14` |
| `price` | `int` | 사용 | clothes 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/clothes.entity.ts:30`, `backend/fxco-hmi/src/clothes/clothes.controller.ts:14` |
| `web_site` | `text` | 사용 | clothes 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/clothes.entity.ts:34`, `backend/fxco-hmi/src/clothes/clothes.controller.ts:14` |

#### `hmi_content`
- 역할: HMI 콘텐츠 메타데이터
- 사용 여부: 사용
- 근거: `backend/fxco-hmi/src/entities/hmi-content.entity.ts:4`, `backend/fxco-hmi/src/hmi-content/hmi-content.service.ts:14`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, 단건 조회 키 | `backend/fxco-hmi/src/entities/hmi-content.entity.ts:7`, `backend/fxco-hmi/src/hmi-content/hmi-content.service.ts:18` |
| `title` | `varchar` | 사용 | 콘텐츠 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/hmi-content.entity.ts:11`, `backend/fxco-hmi/src/hmi-content/hmi-content.controller.ts:14` |
| `code` | `varchar` | 사용 | 콘텐츠 목록/단건 API 응답 필드 | `backend/fxco-hmi/src/entities/hmi-content.entity.ts:15`, `backend/fxco-hmi/src/hmi-content/hmi-content.controller.ts:14` |

#### `rack`
- 역할: rack 마스터 및 hangerleg 위치 기준
- 사용 여부: 사용
- 근거: `backend/fxco-hmi/docs/migration-add-rack-table.sql:7`, `backend/fxco-hmi/src/entities/rack.entity.ts:5`, `backend/fxco-hmi/src/rack/rack.service.ts:14`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, rack 단건 조회 키 | `backend/fxco-hmi/docs/migration-add-rack-table.sql:8`, `backend/fxco-hmi/src/rack/rack.service.ts:18` |
| `rack_number` | `varchar(50)` | 사용 | rack 번호 기반 조회 | `backend/fxco-hmi/docs/migration-add-rack-table.sql:9`, `backend/fxco-hmi/src/rack/rack.service.ts:21` |
| `rack_location` | `varchar(255)` | 사용 | rack 목록/단건 API 응답 필드 | `backend/fxco-hmi/docs/migration-add-rack-table.sql:10`, `backend/fxco-hmi/src/rack/rack.controller.ts:17` |
| `created_at` | `datetime` | 사용 | DDL 기본 생성시각 컬럼, 엔티티 매핑 | `backend/fxco-hmi/docs/migration-add-rack-table.sql:11`, `backend/fxco-hmi/src/entities/rack.entity.ts:20` |

---

## backend/smarthanger_fxco_iot/smarthanger_fxco_iot
### 1) 데이터베이스 개요
- 데이터베이스: `MySQL` (`cc_nanaland_mvp`)
- 문자셋/콜레이션: `미확인`
- 접근 방식: `NestJS + TypeORM`
- 주요 도메인: `행거: hanger, hangerleg, hanger_log` / `의류: clothes` / `시간추적: time_check`

### 2) 테이블 사용 현황 요약
| 테이블명 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|
| `hanger` | 사용 | 연결/픽업/픽다운/의류부착/상태조회 중심 테이블 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:13`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:20`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:17` |
| `hangerleg` | 사용 | 위치 UUID 조회 및 상태 매핑 기준 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-leg.entity.ts:5`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:44`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts:19` |
| `hanger_log` | 사용 | PICKUP/PICKDOWN 로그 저장 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts:12`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:78`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:104` |
| `clothes` | 사용 | 태그 기반 의류 조회 및 hanger 응답 데이터 제공 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:11`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:23`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:34` |
| `time_check` | 사용 | 행거별 착장 시간 기록 저장 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:5`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:86`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:99` |

### 3) 테이블 상세
#### `hanger`
- 역할: 행거 디바이스 상태 및 의류/위치 참조
- 사용 여부: 사용
- 근거: `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:13`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:74`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, 상태 응답 구성 시 참조 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:15`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts:36` |
| `uuid` | `varchar(50)` | 사용 | 연결/픽업/픽다운/의류부착 조회 키 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:18`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:21`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:18` |
| `clothes_seq` | `int` | 사용 | hanger-clothes relation FK | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:22`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:36`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:31` |
| `hangerleg_seq` | `int` | 사용 | 기대 위치(seq) 참조 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:25`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts:42` |
| `nfc_active` | `tinyint` | 사용 | 픽업/픽다운 상태 플래그 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:28`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:53`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:100` |
| `last_pickdown_uuid` | `varchar(255)` | 사용 | 현재 감지 위치 UUID, 충돌 해소/상태매핑 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts:31`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:62`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts:29` |

#### `hangerleg`
- 역할: 위치 태그 UUID 마스터
- 사용 여부: 사용
- 근거: `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-leg.entity.ts:5`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts:19`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, 상태 응답(`hangerLegSeq`) | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-leg.entity.ts:7`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts:48` |
| `uuid` | `varchar(50)` | 사용 | pickdown 입력 UUID 조회, 상태 매핑 키 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-leg.entity.ts:10`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:44`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts:34` |

#### `hanger_log`
- 역할: 행거 이벤트 로그
- 사용 여부: 사용
- 근거: `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts:12`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:78`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK(자동 생성) | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts:14`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:78` |
| `hanger_seq` | `int` | 사용 | 로그의 행거 FK | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts:17`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:79` |
| `hangerleg_seq` | `int` | 사용 | 로그의 leg FK (pickdown) | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts:20`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:80` |
| `self_written` | `tinyint` | 사용 | 정상/시스템 보정 로그 구분 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts:28`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:82`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:138` |
| `created_at` | `datetime` | 사용 | 생성시각 자동 기록 컬럼 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts:39`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:78` |

#### `clothes`
- 역할: 의류 태그/속성 마스터
- 사용 여부: 사용
- 근거: `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:11`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:23`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK, 동일 의류 비교 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:13`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:27` |
| `name` | `varchar(255)` | 판단불가 | 엔티티 정의는 있으나 서비스 컬럼 단위 참조 없음 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:16`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:23` |
| `tag` | `varchar(255)` | 사용 | 태그 기반 조회 키 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:19`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:23` |
| `media` | `int` | 판단불가 | 엔티티 정의는 있으나 서비스 컬럼 단위 참조 없음 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:22`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts:23` |
| `size_color_options` | `json` | 사용 | connect 응답 payload에 포함 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:25`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:34` |
| `price` | `int` | 사용 | connect 응답 payload에 포함 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:28`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:35` |
| `web_site` | `text` | 사용 | connect 응답 payload에 포함 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts:31`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts:36` |

#### `time_check`
- 역할: 행거 사용 시간(ms) 기록
- 사용 여부: 사용
- 근거: `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:5`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:86`
| 컬럼명 | 타입 | 사용 여부 | 역할/미사용 이유 | 근거 |
|---|---|---|---|---|
| `seq` | `int` | 사용 | PK(자동 생성) | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:7`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:97` |
| `hanger_seq` | `int` | 사용 | 대상 행거 FK 저장 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:10`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:87` |
| `hangerleg_seq` | `int` | 사용 | 대상 leg FK 저장 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:13`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:88` |
| `start_at` | `int` | 사용 | 시작 ms 저장 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:17`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:89` |
| `end_at` | `int` | 사용 | 종료 ms 저장 | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:21`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:90` |
| `request_at` | `datetime(3)` | 사용 | 요청시각 저장(명시 없으면 DB 기본값 사용) | `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts:25`, `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts:95` |

---

## DB 미사용 프로젝트
- `app_FxcoDeviceBinder`: DB 미사용/미확인 (근거: `app_FxcoDeviceBinder/pubspec.yaml:30` - DB/ORM 패키지 없음, `app_FxcoDeviceBinder/pubspec.yaml:39` - HTTP/MQTT 중심)
- `app_SmartHangerHmi`: DB 미사용/미확인 (근거: `app_SmartHangerHmi/pubspec.yaml:30` - DB/ORM 패키지 없음)
- `React_new`: DB 미사용/미확인 (근거: `React_new/package.json:14` - React/Express/MQTT 의존성만 존재)
- `HangerClothQRWebSite`: DB 미사용/미확인 (근거: `HangerClothQRWebSite/README.md:1` - 정적 QR 웹 페이지, 서버/DB 설정 파일 부재)
- `IoT`: DB 미사용/미확인 (근거: `IoT/platformio.ini:5` - Arduino 라이브러리 목록, DB 드라이버/ORM 없음)
- `IoT-hangerTest`: DB 미사용/미확인 (근거: `IoT-hangerTest/platformio.ini:15` - MQTT/NTP/JSON 중심)

---

## 기준(근거 파일)
- `backend/fxco-hmi/docs/migration-add-rack-table.sql`
- `backend/fxco-hmi/src/app.module.ts`
- `backend/fxco-hmi/src/entities/clothes.entity.ts`
- `backend/fxco-hmi/src/entities/hanger.entity.ts`
- `backend/fxco-hmi/src/entities/hangerleg.entity.ts`
- `backend/fxco-hmi/src/entities/hanger-log.entity.ts`
- `backend/fxco-hmi/src/entities/hmi-content.entity.ts`
- `backend/fxco-hmi/src/entities/rack.entity.ts`
- `backend/fxco-hmi/src/hanger/hanger.service.ts`
- `backend/fxco-hmi/src/hangerleg/hangerleg.service.ts`
- `backend/fxco-hmi/src/clothes/clothes.service.ts`
- `backend/fxco-hmi/src/hmi-content/hmi-content.service.ts`
- `backend/fxco-hmi/src/rack/rack.service.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/app.module.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/clothes.entity.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger.entity.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-leg.entity.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/hanger-log.entity.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/entities/time-check.entity.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hanger/hanger.service.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/hangerleg/hangerleg.service.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/clothes/clothes.service.ts`
- `backend/smarthanger_fxco_iot/smarthanger_fxco_iot/src/time-tracking/time-tracking.service.ts`
- `app_FxcoDeviceBinder/pubspec.yaml`
- `app_SmartHangerHmi/pubspec.yaml`
- `React_new/package.json`
- `HangerClothQRWebSite/README.md`
- `IoT/platformio.ini`
- `IoT-hangerTest/platformio.ini`

## 검증 결과
- 한글 인코딩 점검: 정상
- 전체 테이블 수: 11
- 사용 11 / 미사용 0 / 판단불가 0 (테이블 기준)

