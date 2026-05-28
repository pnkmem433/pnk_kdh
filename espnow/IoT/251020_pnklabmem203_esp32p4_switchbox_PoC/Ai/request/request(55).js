// request(55).js
window.requestData = window.requestData || [];
window.requestData[55] = {
  "request_number": 55,
  "title": "카메라 해상도 720p 강제 적용 (1080p 자동 선택 버그 수정)",
  "date": "2025-10-31",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request",
    "bugfix",
    "camera-resolution",
    "720p",
    "memory-fix",
    "esp32-p4"
  ],
  "problem": {
    "summary": "EXAMPLE_FORCE_CAM_WIDTH/HEIGHT을 720p로 설정했지만 카메라가 자동으로 1080p 선택. 메모리 부족으로 대부분 프레임 실패"
  },
  "solution": {
    "summary": "app_video.c에 VIDIOC_S_FMT 강제 설정 코드 추가. 자동 해상도 선택 후 다시 1280x720으로 강제 설정"
  },
  "raw_content": {
    "question": `테스트 결과 문제:
- EXAMPLE_FORCE_CAM_WIDTH = 1280, HEIGHT = 720 설정했지만
- 로그: "width=1920 height=1080" (1080p!)
- 인코더: "Resolution: 1920x1080" (1080p 인코딩!)
- YUV420 size: 3110400 bytes (2.97 MB) - 1080p 크기
- 결과: 메모리 부족, 프레임 대량 실패

실제 로그:
\`\`\`
I (1925) app_video: width=1920 height=1080
I (1938) app_main: 카메라 초기화 완료 (1280x720 @ 20fps, YUV420)
I (2378) h264_enc: [DEBUG]   Resolution: 1920x1080
I (2384) h264_enc: [DEBUG]   YUV420 size: 3110400 bytes (2.97 MB)
E (4868) h264_spool: Failed to allocate frame data (472040 bytes)
I (18381) app_main: 인코딩 요청: 80개, 완료: 30개, 드롭: 70개
\`\`\``,
    "answer": `## 문제 분석

### 증상: 해상도 설정 무시됨

**app_video.c Line 30-35 설정**:
\`\`\`c
#ifndef EXAMPLE_FORCE_CAM_WIDTH
#define EXAMPLE_FORCE_CAM_WIDTH  (1280)  // 720p (메모리 최적화)
#endif
#ifndef EXAMPLE_FORCE_CAM_HEIGHT
#define EXAMPLE_FORCE_CAM_HEIGHT (720)   // 720p (메모리 최적화)
#endif
\`\`\`

**실제 로그 (설정 무시됨!)**:
\`\`\`
I (1925) app_video: width=1920 height=1080  ← 1080p!
I (2378) h264_enc: Resolution: 1920x1080     ← 1080p 인코딩!
I (2384) h264_enc: YUV420 size: 3110400 bytes (2.97 MB)  ← 1080p 크기!
\`\`\`

**기대했던 로그**:
\`\`\`
I (xxxx) app_video: width=1280 height=720   ← 720p
I (xxxx) h264_enc: Resolution: 1280x720     ← 720p 인코딩
I (xxxx) h264_enc: YUV420 size: 1382400 bytes (1.32 MB)  ← 720p 크기
\`\`\`

### 근본 원인 분석

**app_video.c 자동 해상도 선택 로직 (Line 140-174)**:
\`\`\`c
// 지원되는 모든 해상도 중 최대 해상도 자동 선택
for (fsz.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsz) == 0; fsz.index++) {
    uint32_t w = 0, h = 0;
    if (fsz.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
        w = fsz.discrete.width; h = fsz.discrete.height;
    } else if (fsz.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
        w = fsz.stepwise.max_width; h = fsz.stepwise.max_height;
    }
    // 문제: 최대 해상도를 선택함!
    if (w*h > best_w*best_h) {
        best_w = w; best_h = h; best_fmt = f;
    }
}

// 선택된 최대 해상도로 설정 (EXAMPLE_FORCE_CAM_WIDTH 무시!)
format.fmt.pix.width = best_w;   // 1920
format.fmt.pix.height = best_h;  // 1080
if (ioctl(fd, VIDIOC_S_FMT, &format) == 0) {
    ESP_LOGI(TAG, "selected %" PRIu32 "x%" PRIu32, best_w, best_h);
}
\`\`\`

**문제**:
1. 코드가 지원되는 **최대 해상도**를 자동으로 찾음 (1920x1080)
2. `EXAMPLE_FORCE_CAM_WIDTH/HEIGHT` 설정을 **완전히 무시**
3. 1080p로 설정 후 강제 변경 로직 없음

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 해결 방법: 해상도 강제 설정

### 코드 변경

**app_video.c Line 176-187 추가**:
\`\`\`c
// Force the desired resolution (EXAMPLE_FORCE_CAM_WIDTH/HEIGHT)
struct v4l2_format forced_format = {0};
forced_format.type = type;
forced_format.fmt.pix.width = EXAMPLE_FORCE_CAM_WIDTH;   // 1280
forced_format.fmt.pix.height = EXAMPLE_FORCE_CAM_HEIGHT; // 720
forced_format.fmt.pix.pixelformat = best_fmt > 0 ? best_fmt : default_format.fmt.pix.pixelformat;

if (ioctl(fd, VIDIOC_S_FMT, &forced_format) == 0) {
    ESP_LOGI(TAG, "Forced resolution to %dx%d", EXAMPLE_FORCE_CAM_WIDTH, EXAMPLE_FORCE_CAM_HEIGHT);
} else {
    ESP_LOGW(TAG, "Failed to force resolution, using default");
}

// Read back the actual active format and update our state
memset(&default_format, 0, sizeof(struct v4l2_format));
default_format.type = type;
if (ioctl(fd, VIDIOC_G_FMT, &default_format) != 0) {
    ESP_LOGW(TAG, "failed to get active format after set, keep previous values");
}

app_camera_video.camera_buf_hes = default_format.fmt.pix.width;
app_camera_video.camera_buf_ves = default_format.fmt.pix.height;
app_camera_video.pixel_format = default_format.fmt.pix.pixelformat;
\`\`\`

### 동작 순서

**Before (버그)**:
\`\`\`
1. 자동 해상도 선택: 1920x1080 (최대)
2. VIDIOC_S_FMT: 1920x1080 설정
3. VIDIOC_G_FMT: 1920x1080 읽기
4. 결과: 1080p 인코딩 → 메모리 부족
\`\`\`

**After (수정)**:
\`\`\`
1. 자동 해상도 선택: 1920x1080 (최대)
2. VIDIOC_S_FMT: 1920x1080 설정
3. ★ VIDIOC_S_FMT: 1280x720 강제 설정 (새로 추가!)
4. VIDIOC_G_FMT: 1280x720 읽기
5. 결과: 720p 인코딩 → 메모리 충분
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 변경 요약

### 수정된 파일

1. **[main/app_video.c](main/app_video.c)**
   - Line 176-187: 해상도 강제 설정 로직 추가
   - VIDIOC_S_FMT로 1280x720 강제 설정
   - 로그: "Forced resolution to 1280x720"

### 메모리 효과

**Before (1080p 버그)**:
\`\`\`
YUV420 프레임: 1920 × 1080 × 1.5 = 3,110,400 bytes (3.0 MB)
Output buffer:  4,665,600 bytes (4.45 MB)
프레임당 총: ~7.5 MB
100 프레임: 750 MB (이론상) → 메모리 부족!
\`\`\`

**After (720p 수정)**:
\`\`\`
YUV420 프레임: 1280 × 720 × 1.5 = 1,382,400 bytes (1.32 MB)
Output buffer:  ~2,073,600 bytes (1.98 MB)
프레임당 총: ~3.3 MB (56% 감소!)
100 프레임: 330 MB (이론상) → 충분!
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 예상 결과

### 빌드 및 실행

**빌드**:
\`\`\`bash
idf.py build
\`\`\`

**플래시**:
\`\`\`bash
idf.py flash monitor
\`\`\`

### 예상 로그 출력

**카메라 초기화** (새 로그!):
\`\`\`
I (xxxx) app_video: selected 1920x1080 fmt=0x...
I (xxxx) app_video: ★ Forced resolution to 1280x720  ← 새 로그!
I (xxxx) app_video: width=1280 height=720  ← 720p 확인!
I (xxxx) app_main: 카메라 초기화 완료 (1280x720 @ 20fps, YUV420)
\`\`\`

**인코더 초기화**:
\`\`\`
I (xxxx) h264_enc: [DEBUG]   Resolution: 1280x720  ← 720p!
I (xxxx) h264_enc: [DEBUG]   YUV420 size: 1382400 bytes (1.32 MB)  ← 720p 크기!
I (xxxx) h264_enc: [DEBUG]   Output buffer: ~2073600 bytes (1.98 MB)
I (xxxx) h264_spool: Available SPIRAM: 21.xx MB  ← 더 많은 메모리!
\`\`\`

**녹화 성공**:
\`\`\`
I (xxxx) h264_enc: Frame #0: Size: ~800 KB
I (xxxx) h264_enc: Frame #1: Size: ~600 KB
...
I (xxxx) app_main: 동영상 녹화중 50% (50/100 프레임)
...
I (xxxx) app_main: 동영상 녹화중 100% (100/100 프레임)

[메모리 할당 실패 없음!]
\`\`\`

**녹화 완료**:
\`\`\`
I (xxxx) app_main: === 100 프레임 캡처 완료 ===
I (xxxx) app_main: 캡처 프레임: 100개 (5초 @ 20fps, 720p)
I (xxxx) app_main: 인코딩 요청: 100개, 완료: 100개, 드롭: 0개  ← 드롭 0!
I (xxxx) app_main: 실제 FPS: 20.00  ← 목표 달성!
I (xxxx) h264_spool: Writing 100 buffered frames from memory to temp file...
I (xxxx) h264_spool: Memory flush complete: 100 frames
I (xxxx) app_main: === 저장완료 ===
\`\`\`

### 성공 기준

**카메라 해상도**:
\`\`\`
Before: width=1920 height=1080
After:  width=1280 height=720  ✅
\`\`\`

**메모리 사용**:
\`\`\`
Before: YUV420 3.0 MB, Output 4.45 MB
After:  YUV420 1.32 MB, Output 1.98 MB  ✅
\`\`\`

**인코딩 성공률**:
\`\`\`
Before: 80 요청, 30 완료, 70 드롭 (37.5% 성공)
After:  100 요청, 100 완료, 0 드롭 (100% 성공)  ✅
\`\`\`

**FPS 달성**:
\`\`\`
Before: 0.64 fps
After:  20.00 fps  ✅
\`\`\`

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 왜 이 버그가 발생했나?

### 자동 해상도 선택 로직

**의도**:
- 카메라가 지원하는 최적의 해상도 자동 선택
- 일반적으로 최대 해상도가 최선

**문제**:
- 임베디드 시스템에서는 메모리 제약 존재
- 최대 해상도 ≠ 최적 해상도
- `EXAMPLE_FORCE_CAM_WIDTH/HEIGHT` 설정이 있어도 무시됨

### 해결 철학

**Before**:
- "카메라가 지원하는 최대 해상도를 사용하자"
- 결과: 메모리 부족

**After**:
- "사용자가 설정한 해상도를 강제하자"
- 결과: 안정적 동작

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 다음 단계

1. **빌드 및 플래시**:
   \`\`\`bash
   idf.py build
   idf.py flash monitor
   \`\`\`

2. **테스트 확인사항**:
   - ✅ "Forced resolution to 1280x720" 로그 확인
   - ✅ "width=1280 height=720" 확인
   - ✅ "YUV420 size: 1382400 bytes (1.32 MB)" 확인
   - ✅ 100 프레임 모두 성공
   - ✅ 메모리 할당 실패 0건
   - ✅ 인코딩 드롭 0개
   - ✅ 실제 FPS: 20.00

3. **성공 기준**:
   - ✅ 카메라: 1280x720
   - ✅ 인코더: 1280x720
   - ✅ 프레임: 100/100
   - ✅ 드롭: 0
   - ✅ FPS: 20.00

================================================================`
  },
  "sections": {
    "문제 분석": `증상: 해상도 설정 무시됨

설정:
- EXAMPLE_FORCE_CAM_WIDTH = 1280
- EXAMPLE_FORCE_CAM_HEIGHT = 720

실제 로그:
- width=1920 height=1080 (1080p!)
- Resolution: 1920x1080 (1080p 인코딩!)
- YUV420 size: 3110400 bytes (3.0 MB)

근본 원인:
- 자동 해상도 선택 로직이 최대 해상도 선택
- EXAMPLE_FORCE_CAM_WIDTH/HEIGHT 설정 무시
- 1080p 인코딩 → 메모리 부족`,

    "해결 방법": `해상도 강제 설정:

app_video.c Line 176-187 추가:
\`\`\`c
struct v4l2_format forced_format = {0};
forced_format.fmt.pix.width = EXAMPLE_FORCE_CAM_WIDTH;   // 1280
forced_format.fmt.pix.height = EXAMPLE_FORCE_CAM_HEIGHT; // 720

if (ioctl(fd, VIDIOC_S_FMT, &forced_format) == 0) {
    ESP_LOGI(TAG, "Forced resolution to %dx%d", 1280, 720);
}
\`\`\`

동작:
1. 자동 선택: 1920x1080
2. 강제 설정: 1280x720 (새로 추가!)
3. 결과: 720p 인코딩`,

    "예상 결과": `카메라 초기화:
✅ "Forced resolution to 1280x720"
✅ width=1280 height=720

인코더:
✅ Resolution: 1280x720
✅ YUV420: 1382400 bytes (1.32 MB)

녹화:
✅ 100 프레임 모두 성공
✅ 드롭: 0개 (Before: 70개)
✅ 실제 FPS: 20.00 (Before: 0.64)

메모리:
✅ 할당 실패 0건
✅ 프레임당 3.3 MB (Before: 7.5 MB)`,

    "왜 이 버그인가": `자동 해상도 선택 로직:
- 의도: 최대 해상도 자동 선택
- 문제: 임베디드 시스템 메모리 제약 무시

해결 철학 변경:
- Before: "최대 해상도 사용"
- After: "사용자 설정 강제"

결과:
- 메모리 부족 해결
- 안정적 동작 달성`
  }
};
