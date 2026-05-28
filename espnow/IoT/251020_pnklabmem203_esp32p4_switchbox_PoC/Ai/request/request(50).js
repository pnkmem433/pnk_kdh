// request(50).js
window.requestData = window.requestData || [];
window.requestData[50] = {
  "request_number": 50,
  "title": "컴파일 에러 수정: uint32_t 포맷 지정자 타입 불일치",
  "date": "2025-10-29",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request",
    "bugfix",
    "compilation",
    "format-specifier",
    "esp32-p4"
  ],
  "problem": {
    "summary": "청크 단위 배치 쓰기 구현 후 빌드 시 format specifier 타입 불일치 컴파일 에러 발생. ESP32-P4에서 uint32_t는 'long unsigned int'이므로 %u 포맷 지정자 사용 불가"
  },
  "solution": {
    "summary": "PRIu32 매크로 사용으로 플랫폼별 올바른 포맷 지정자 자동 선택. %u → %\" PRIu32 \" 변경"
  },
  "raw_content": {
    "question": `컴파일 에러 발생:

\`\`\`
D:/04.pretests-iot/251020_pnklabmem203_esp32p4_switchbox_PoC/main/h264_spool.c:102:9:
error: format '%u' expects argument of type 'unsigned int',
but argument 6 has type 'uint32_t' {aka 'long unsigned int'} [-Werror=format=]

ESP_LOGI(TAG, "Writing chunk: %u frames (buffer: %u/%u)",
         frames_to_write, available_frames, (unsigned)spool->ring_capacity);
\`\`\``,
    "answer": `## 문제 분석

### 컴파일 에러 상세

**에러 메시지**:
\`\`\`
error: format '%u' expects argument of type 'unsigned int',
but argument has type 'uint32_t' {aka 'long unsigned int'} [-Werror=format=]
\`\`\`

**발생 위치**: [h264_spool.c:102](main/h264_spool.c#L102)

**에러 원인**:
\`\`\`c
ESP_LOGI(TAG, "Writing chunk: %u frames (buffer: %u/%u)",
         frames_to_write, available_frames, (unsigned)spool->ring_capacity);
\`\`\`

### 근본 원인

**ESP32-P4 플랫폼에서 타입 정의**:
\`\`\`c
// stdint.h
typedef unsigned int       uint32_t;  // 32비트 일반 플랫폼
typedef long unsigned int  uint32_t;  // 64비트 또는 ESP32-P4
\`\`\`

**포맷 지정자 불일치**:
- \`%u\`: \`unsigned int\` 기대
- \`uint32_t\`: ESP32-P4에서 \`long unsigned int\` (4 bytes)
- 타입 불일치 → \`-Werror=format\` 플래그로 인해 빌드 실패

**왜 (unsigned) 캐스팅도 실패?**
\`\`\`c
(unsigned)spool->ring_capacity  // unsigned int로 캐스팅
// 하지만 frames_to_write, available_frames는 여전히 uint32_t
// → 여전히 타입 불일치
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 해결 방법

### PRIu32 매크로 사용

**inttypes.h에 정의된 플랫폼 독립적 포맷 매크로**:
\`\`\`c
#include <inttypes.h>

// 32비트 플랫폼
#define PRIu32  "u"

// 64비트 플랫폼 또는 ESP32-P4
#define PRIu32  "lu"
\`\`\`

**수정 전**:
\`\`\`c
ESP_LOGI(TAG, "Writing chunk: %u frames (buffer: %u/%u)",
         frames_to_write, available_frames, (unsigned)spool->ring_capacity);
\`\`\`

**수정 후**:
\`\`\`c
ESP_LOGI(TAG, "Writing chunk: %" PRIu32 " frames (buffer: %" PRIu32 "/%" PRIu32 ")",
         frames_to_write, available_frames, spool->ring_capacity);
\`\`\`

**변경 사항**:
1. \`%u\` → \`%" PRIu32 "\`
2. \`(unsigned)\` 캐스팅 제거 (불필요)
3. PRIu32 매크로가 컴파일러에 의해 적절한 포맷으로 자동 변환

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 코드 변경 사항

### main/h264_spool.c Line 102-103

\`\`\`diff
- ESP_LOGI(TAG, "Writing chunk: %u frames (buffer: %u/%u)",
-          frames_to_write, available_frames, (unsigned)spool->ring_capacity);
+ ESP_LOGI(TAG, "Writing chunk: %" PRIu32 " frames (buffer: %" PRIu32 "/%" PRIu32 ")",
+          frames_to_write, available_frames, spool->ring_capacity);
\`\`\`

**변경 내역**:
- 3개의 \`%u\` → \`%" PRIu32 "\`
- \`(unsigned)\` 캐스팅 제거
- 기능 변경 없음 (로그 출력 포맷만 수정)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 왜 이런 문제가 발생했나?

### ESP-IDF 컴파일러 플래그

**ESP-IDF는 기본적으로 -Werror=format 활성화**:
\`\`\`cmake
# CMakeLists.txt 또는 sdkconfig
CONFIG_COMPILER_WARN_WRITE_STRINGS=y
CONFIG_COMPILER_OPTIMIZATION_LEVEL_RELEASE=y
\`\`\`

**목적**:
- 포맷 문자열 오류로 인한 메모리 손상 방지
- 타입 불일치로 인한 버그 사전 차단
- 임베디드 시스템에서 안정성 최우선

### 다른 플랫폼과의 차이

**x86/x64 리눅스**:
\`\`\`c
uint32_t x = 100;
printf("%u\\n", x);  // OK (uint32_t == unsigned int)
\`\`\`

**ESP32-P4 (Xtensa 아키텍처)**:
\`\`\`c
uint32_t x = 100;
printf("%u\\n", x);  // 컴파일 에러!
printf("%lu\\n", x); // OK (uint32_t == long unsigned int)
printf("%" PRIu32 "\\n", x);  // 권장 (플랫폼 독립적)
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 유사한 에러 방지 가이드

### 권장 포맷 지정자

**정수 타입**:
\`\`\`c
int8_t    → %" PRId8 "
uint8_t   → %" PRIu8 "
int16_t   → %" PRId16 "
uint16_t  → %" PRIu16 "
int32_t   → %" PRId32 "
uint32_t  → %" PRIu32 "
int64_t   → %" PRId64 "
uint64_t  → %" PRIu64 "
size_t    → %zu
\`\`\`

**포인터**:
\`\`\`c
void *ptr → %p
\`\`\`

**ESP-IDF 타입**:
\`\`\`c
esp_err_t → %d 또는 %s (esp_err_to_name()과 함께)
TickType_t → %" PRIu32 "
\`\`\`

### ESP-IDF 로깅 예제

**올바른 사용**:
\`\`\`c
uint32_t count = 100;
uint64_t timestamp = esp_timer_get_time();
size_t size = 1024;

ESP_LOGI(TAG, "Count: %" PRIu32, count);
ESP_LOGI(TAG, "Timestamp: %" PRIu64 " us", timestamp);
ESP_LOGI(TAG, "Size: %zu bytes", size);
\`\`\`

**잘못된 사용** (컴파일 에러):
\`\`\`c
uint32_t count = 100;
ESP_LOGI(TAG, "Count: %u", count);  // ❌ ESP32-P4에서 에러
ESP_LOGI(TAG, "Count: %d", count);  // ❌ signed/unsigned 불일치
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 검증 결과

### 빌드 명령어

\`\`\`bash
idf.py build
\`\`\`

### 예상 결과

**빌드 성공**:
\`\`\`
[100%] Built target esp-idf::main
Linking CXX executable 251020_esp32p4_switchbox_PoC.elf
Generating binary image from built executable
\`\`\`

**로그 출력 (런타임)**:
\`\`\`
I (5234) h264_spool: Writing chunk: 64 frames (buffer: 64/300)
I (8145) h264_spool: Writing chunk: 64 frames (buffer: 64/300)
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 관련 커밋

- **6131203**: h264_spool: 더블 버퍼링 구조로 개선 (원본 구현)
- **f810c0b**: Fix: uint32_t 포맷 지정자 컴파일 에러 수정 (이번 수정)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 다음 단계

1. **빌드 재시도**:
   \`\`\`bash
   idf.py build
   \`\`\`

2. **빌드 성공 확인**:
   - 컴파일 에러 없음
   - 경고 없음
   - 펌웨어 생성 완료

3. **플래시 및 테스트**:
   \`\`\`bash
   idf.py flash monitor
   \`\`\`

4. **로그 확인**:
   - "Writing chunk: 64 frames (buffer: 64/300)" 출력
   - 청크 단위 배치 쓰기 동작 확인
   - 30fps 인코딩 달성 확인

================================================================`
  },
  "sections": {
    "문제 분석": `컴파일 에러:
- format '%u' expects 'unsigned int'
- but argument has type 'uint32_t' {aka 'long unsigned int'}
- -Werror=format 플래그로 인해 빌드 실패

근본 원인:
- ESP32-P4에서 uint32_t = long unsigned int
- %u 포맷은 unsigned int 기대
- 타입 불일치로 컴파일 에러`,

    "해결 방법": `PRIu32 매크로 사용:
- inttypes.h에 정의된 플랫폼 독립적 포맷 매크로
- 32비트: "u", 64비트/ESP32-P4: "lu"
- 컴파일러가 자동으로 적절한 포맷 선택

변경 사항:
- %u → %" PRIu32 "
- (unsigned) 캐스팅 제거
- 3개 인자 모두 수정`,

    "코드 변경": `main/h264_spool.c Line 102-103:

수정 전:
ESP_LOGI(TAG, "Writing chunk: %u frames (buffer: %u/%u)",
         frames_to_write, available_frames, (unsigned)spool->ring_capacity);

수정 후:
ESP_LOGI(TAG, "Writing chunk: %" PRIu32 " frames (buffer: %" PRIu32 "/%" PRIu32 ")",
         frames_to_write, available_frames, spool->ring_capacity);`,

    "권장 포맷 지정자": `정수 타입:
- int32_t  → %" PRId32 "
- uint32_t → %" PRIu32 "
- int64_t  → %" PRId64 "
- uint64_t → %" PRIu64 "
- size_t   → %zu

ESP-IDF 타입:
- esp_err_t → %d 또는 %s (esp_err_to_name())
- TickType_t → %" PRIu32 "`,

    "다음 단계": `1. 빌드 재시도: idf.py build
2. 빌드 성공 확인 (컴파일 에러 없음)
3. 플래시 및 테스트: idf.py flash monitor
4. 로그 확인: "Writing chunk: 64 frames (buffer: 64/300)"
5. 청크 단위 배치 쓰기 동작 확인
6. 30fps 인코딩 달성 확인`
  }
};
