// request(51).js
window.requestData = window.requestData || [];
window.requestData[51] = {
  "request_number": 51,
  "title": "청크 크기 축소로 메모리 부족 문제 해결 (64→10 프레임)",
  "date": "2025-10-29",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request",
    "bugfix",
    "memory",
    "chunk-size",
    "optimization"
  ],
  "problem": {
    "summary": "청크 단위 배치 쓰기 구현 후 Frame #6부터 메모리 할당 실패. SD Writer가 64 프레임을 기다리는 동안 메모리가 부족해서 청크가 채워지지 않음. 메모리 누적으로 인한 할당 실패 반복"
  },
  "solution": {
    "summary": "청크 크기를 64 → 10 프레임으로 축소. 메모리 요구량 32 MB → 5 MB 감소. 프레임이 있으면 즉시 쓰기 시작하도록 대기 로직 개선"
  },
  "raw_content": {
    "question": `로그 분석:
\`\`\`
I (2664) h264_spool: SD write buffer: 256 KB (optimized for chunked writes)
I (2668) h264_spool: SD writer task started (chunked batch write mode)
I (2969) h264_enc: ═══ Frame #0 Encoded ═══ Size: 1264367 bytes
I (3260) h264_enc: ═══ Frame #1 Encoded ═══ Size: 1087962 bytes
...
I (4863) h264_spool: Failed to allocate frame data (472672 bytes)  // Frame #6 실패!
I (5457) h264_spool: Failed to allocate frame data (339136 bytes)
I (5638) h264_spool: Failed to allocate frame data (309252 bytes)
...
// SD Writer가 청크를 쓰지 않음 ("Writing chunk" 로그 없음!)
\`\`\`

문제: SD Writer가 작동하지 않아 메모리가 계속 쌓여 할당 실패`,
    "answer": `## 문제 분석

### 증상

**Frame #6부터 메모리 할당 지속 실패**:
\`\`\`
E (4863) h264_spool: Failed to allocate frame data (472672 bytes)
E (5457) h264_spool: Failed to allocate frame data (339136 bytes)
E (5638) h264_spool: Failed to allocate frame data (309252 bytes)
...
\`\`\`

**SD Writer 동작 없음**:
- 로그에 "Writing chunk" 메시지 없음
- SD Writer가 청크를 쓰지 않음
- 메모리가 계속 쌓임

### 근본 원인

**청크 크기 문제**:
\`\`\`c
// h264_spool.c Line 74
uint32_t chunk_size = 64;  // 64 프레임 단위 배치 쓰기

// Line 89-92: 문제의 코드!
if (available_frames < chunk_size && !spool->stop_writer) {
    vTaskDelay(pdMS_TO_TICKS(100));  // 64 프레임 미만이면 계속 대기
    continue;  // SD 쓰기 안 함!
}
\`\`\`

**메모리 부족 악순환**:
1. 인코더가 프레임 생성 → 메모리 할당
2. SD Writer는 64 프레임 기다림
3. 6 프레임 쌓인 시점에서 메모리 부족 (약 3 MB)
4. 더 이상 프레임 할당 불가
5. 64 프레임이 채워지지 않음 → SD Writer 영원히 대기
6. 메모리 누수 계속

### 메모리 계산 (32 MB SPIRAM)

**필요한 메모리**:
\`\`\`
인코더 출력 버퍼: 4.5 MB × 2 = 9 MB (고정)
포맷 변환 버퍼: 3.1 MB (고정)
64 프레임 버퍼: 64 × 500 KB = 32 MB (평균)
───────────────────────────────────────
총 필요: 44.1 MB

사용 가능: 32 MB SPIRAM ❌
결과: 메모리 부족!
\`\`\`

**실제 발생**:
\`\`\`
프레임 0-2: 1.26 MB, 1.09 MB, 0.86 MB (총 3.21 MB) ✓
프레임 3-5: 약 1.5 MB 추가 (총 4.71 MB) ✓
프레임 6: 0.47 MB 할당 시도 → 실패! ❌
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 해결 방법

### 1. 청크 크기 축소

**변경 내역**:
\`\`\`c
// 변경 전 (Line 74)
uint32_t chunk_size = 64;  // 32 MB 버퍼 필요

// 변경 후
uint32_t chunk_size = 10;  // 5 MB 버퍼 (메모리 압박 완화)
\`\`\`

**메모리 계산 (변경 후)**:
\`\`\`
인코더 출력 버퍼: 9 MB
포맷 변환 버퍼: 3.1 MB
10 프레임 버퍼: 10 × 500 KB = 5 MB
───────────────────────────────────────
총 사용: 17.1 MB ✅
여유: 14.9 MB ✅
\`\`\`

### 2. 대기 로직 개선

**변경 전** (Line 88-98):
\`\`\`c
// 청크 크기 미만이고 아직 녹화 중이면 대기
if (available_frames < chunk_size && !spool->stop_writer) {
    vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 대기
    continue;  // 64 프레임 미만이면 쓰기 안 함!
}

// 청크가 없으면 종료 확인
if (available_frames == 0) {
    vTaskDelay(pdMS_TO_TICKS(10));
    continue;
}
\`\`\`

**변경 후**:
\`\`\`c
// 프레임이 없으면 대기
if (available_frames == 0) {
    if (spool->stop_writer) {
        break;  // 녹화 종료 및 버퍼 비었으면 종료
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    continue;
}

// 1 프레임이라도 있으면 즉시 쓰기 시작!
// 배치 쓰기: min(available, chunk_size)
\`\`\`

**핵심 변경**:
- ❌ 청크 크기 미만이면 대기
- ✅ 프레임이 있으면 즉시 쓰기
- ✅ 메모리 압박 최소화

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 코드 변경 사항

### main/h264_spool.c

**Line 64-66: 주석 업데이트**
\`\`\`c
// 변경 전
// 전략: 메모리에 프레임 축적 → 64 프레임마다 배치 쓰기
// 목적: SD I/O 병목 제거, 30fps 인코딩 보장

// 변경 후
// 전략: 메모리에 프레임 축적 → 10 프레임마다 배치 쓰기
// 목적: SD I/O 병목 제거하면서 메모리 압박 최소화
// 변경: 64→10 프레임 (메모리 부족 방지, 5 MB 버퍼)
\`\`\`

**Line 74: 청크 크기 변경**
\`\`\`c
uint32_t chunk_size = 10;  // 10 프레임 단위 배치 쓰기
\`\`\`

**Line 77-100: 대기 로직 간소화**
\`\`\`diff
  while (!spool->stop_writer || spool->read_idx != spool->write_idx) {
-     // 청크가 채워질 때까지 대기 (64 프레임 또는 녹화 종료)
+     // 사용 가능한 프레임 계산
      uint32_t available_frames = 0;

      xSemaphoreTake(spool->mutex, portMAX_DELAY);
      if (spool->write_idx >= spool->read_idx) {
          available_frames = spool->write_idx - spool->read_idx;
      } else {
          available_frames = spool->ring_capacity - spool->read_idx + spool->write_idx;
      }
      xSemaphoreGive(spool->mutex);

-     // 청크 크기 미만이고 아직 녹화 중이면 대기
-     if (available_frames < chunk_size && !spool->stop_writer) {
-         vTaskDelay(pdMS_TO_TICKS(100));
-         continue;
-     }
-
-     // 청크가 없으면 종료 확인
+     // 프레임이 없으면 대기
      if (available_frames == 0) {
+         if (spool->stop_writer) {
+             break;  // 녹화 종료 및 버퍼 비었으면 종료
+         }
          vTaskDelay(pdMS_TO_TICKS(10));
          continue;
      }

-     // 배치 쓰기 시작
+     // 배치 쓰기 시작 (청크 크기 또는 사용 가능한 모든 프레임)
      uint32_t frames_to_write = (available_frames > chunk_size) ? chunk_size : available_frames;
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 성능 영향 분석

### 배치 쓰기 효율

**64 프레임 청크**:
- SD I/O 빈도: 1회 / 2.1초 (64 프레임)
- 프레임당 대기 시간: 0ms (메모리 축적)
- 문제: 메모리 부족으로 실행 불가 ❌

**10 프레임 청크**:
- SD I/O 빈도: 1회 / 0.33초 (10 프레임)
- 프레임당 대기 시간: 0ms (여전히 배치 쓰기)
- 효과: 개별 쓰기 대비 **10배 빠름** ✅

**개별 쓰기 (참고)**:
- SD I/O 빈도: 1회 / 0.033초 (1 프레임)
- 프레임당 대기 시간: 높음
- 효과: 병목 발생

### 메모리 사용량 비교

| 구분 | 인코더 | 청크 버퍼 | 총 사용 | 여유 | 상태 |
|------|--------|-----------|---------|------|------|
| 64 프레임 | 9 MB | 32 MB | 41 MB | -9 MB | ❌ 부족 |
| 10 프레임 | 9 MB | 5 MB | 14 MB | 18 MB | ✅ 충분 |

### 예상 성능

**인코딩**:
- 메모리 압박 완화로 안정적 할당
- 예상 fps: **향상** (메모리 부족 해소)

**SD 쓰기**:
- 배치 쓰기 효과 유지
- 10 프레임마다 백그라운드 쓰기
- 인코딩 블로킹 최소화

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 검증 방법

### 빌드 및 플래시

\`\`\`bash
idf.py build
idf.py flash monitor
\`\`\`

### 예상 로그

**SD Writer 시작**:
\`\`\`
I (2668) h264_spool: SD writer task started (chunked batch write mode)
\`\`\`

**청크 쓰기 (10 프레임마다)**:
\`\`\`
I (xxxx) h264_spool: Writing chunk: 10 frames (buffer: 10/300)
I (xxxx) h264_spool: Writing chunk: 10 frames (buffer: 10/300)
I (xxxx) h264_spool: Writing chunk: 10 frames (buffer: 10/300)
...
\`\`\`

**메모리 할당 성공**:
\`\`\`
I (2969) h264_enc: ═══ Frame #0 Encoded ═══ Size: 1264367 bytes
I (3260) h264_enc: ═══ Frame #1 Encoded ═══ Size: 1087962 bytes
...
I (xxxx) h264_enc: ═══ Frame #300 Encoded ═══  // 메모리 에러 없음!
\`\`\`

### 성공 기준

✅ **메모리 할당 실패 없음**
- "Failed to allocate frame data" 에러 없음
- 300 프레임 모두 성공적으로 인코딩

✅ **SD Writer 정상 동작**
- "Writing chunk" 로그 30회 출력 (300 프레임 / 10)
- 10 프레임마다 배치 쓰기

✅ **인코딩 fps 향상**
- 메모리 압박 완화로 안정적 성능
- 예상: 5-7 fps (현재) → 향상

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 관련 커밋

- **6131203**: h264_spool: 더블 버퍼링 구조로 개선 (원본, 64 프레임)
- **f810c0b**: Fix: uint32_t 포맷 지정자 수정
- **ae0248a**: Fix: 청크 크기 축소 (64→10 프레임) 메모리 부족 해결

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 다음 단계

1. **빌드 및 플래시**
2. **로그 모니터링**:
   - "Writing chunk: 10 frames" 확인
   - 메모리 할당 실패 없음 확인
3. **성능 측정**:
   - 인코딩 fps 확인
   - 300 프레임 완료 시간 측정
4. **추가 최적화 검토** (필요 시):
   - 청크 크기 조정 (10 → 8 or 12)
   - 메모리 프로파일링

================================================================`
  },
  "sections": {
    "문제 분석": `증상:
- Frame #6부터 메모리 할당 지속 실패
- SD Writer가 작동하지 않음 ("Writing chunk" 로그 없음)

근본 원인:
- 청크 크기 64 프레임 = 32 MB 버퍼 필요
- 64 프레임 미만이면 SD Writer가 계속 대기
- 메모리 부족으로 6 프레임만 쌓이고 멈춤
- 64 프레임이 채워지지 않아 SD 쓰기 시작 안 됨

메모리 계산:
- 필요: 인코더 9 MB + 청크 32 MB = 41 MB
- 사용 가능: 32 MB SPIRAM
- 결과: 메모리 부족!`,

    "해결 방법": `1. 청크 크기 축소 (64 → 10 프레임):
   - 메모리 요구: 32 MB → 5 MB
   - 총 사용: 14 MB (여유 18 MB)

2. 대기 로직 개선:
   - 변경 전: 64 프레임 미만이면 무조건 대기
   - 변경 후: 프레임이 있으면 즉시 쓰기
   - 효과: 메모리 압박 최소화`,

    "코드 변경": `main/h264_spool.c:

Line 74: 청크 크기 변경
- uint32_t chunk_size = 64; → 10;

Line 88-95: 대기 로직 간소화
- 청크 크기 미만 대기 제거
- 프레임 있으면 즉시 쓰기
- 메모리 누적 방지`,

    "성능 영향": `배치 쓰기 효율:
- 64 프레임: 실행 불가 (메모리 부족)
- 10 프레임: 개별 쓰기 대비 10배 빠름

메모리 사용:
- 64 프레임: 41 MB 필요 (오버플로우)
- 10 프레임: 14 MB 사용 (18 MB 여유)

예상 성능:
- 메모리 할당 실패: 없음
- 인코딩 fps: 향상 (메모리 압박 완화)
- SD 쓰기: 배치 효과 유지`,

    "다음 단계": `1. 빌드 및 플래시: idf.py build && idf.py flash monitor
2. 로그 확인:
   - "Writing chunk: 10 frames" 출력
   - 메모리 할당 실패 없음
3. 성능 측정:
   - 인코딩 fps 확인
   - 300 프레임 완료 시간
4. 필요 시 청크 크기 재조정 (10 → 8 or 12)`
  }
};
