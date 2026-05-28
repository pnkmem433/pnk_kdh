// request(58).js
window.requestData = window.requestData || [];
window.requestData[58] = {
  "request_number": 58,
  "title": "sdkconfig로 카메라 해상도 설정 (1280x960 적용)",
  "date": "2025-10-31",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request",
    "camera",
    "sdkconfig",
    "resolution",
    "critical-fix"
  ],
  "problem": {
    "summary": "app_video.c에서 EXAMPLE_FORCE_CAM_WIDTH/HEIGHT 설정해도 카메라가 계속 1920x1080로 동작",
    "previous_attempts": [
      "request(56): VGA 640x480 시도 → 미지원",
      "request(57): 1280x960 코드 변경 → 무시됨",
      "app_video.c에서 VIDIOC_S_FMT로 강제 설정 → 실패"
    ],
    "root_cause": "sdkconfig.defaults에서 CONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS=y로 고정됨",
    "evidence": "로그: I (2391) h264_enc: Resolution: 1920x1080 ← 코드 변경 무시",
    "why_code_ignored": "ESP-IDF는 sdkconfig 설정을 우선하며, 센서 드라이버 초기화 시 CONFIG 값으로 레지스터 설정"
  },
  "investigation": {
    "source": "example/video_lcd_display 공식 예제 분석",
    "key_finding": "example은 sdkconfig.defaults에서 카메라 해상도 설정",
    "example_config": "CONFIG_CAMERA_SC2336_MIPI_RAW8_1024x600_30FPS=y",
    "mechanism": [
      "1. sdkconfig.defaults → 빌드 시스템 CONFIG 정의",
      "2. esp_video_init() → 센서 드라이버 초기화",
      "3. 센서 드라이버 → CONFIG 값으로 레지스터 설정 선택",
      "4. ov5647_settings.h의 해당 테이블 적용",
      "5. VIDIOC_S_FMT는 이미 설정된 해상도 확인만 가능"
    ],
    "why_vidioc_failed": "센서는 이미 초기화 시 레지스터 설정되어 V4L2로 변경 불가"
  },
  "solution": {
    "summary": "sdkconfig.defaults에서 1280x960 해상도 설정",
    "file": "sdkconfig.defaults",
    "changes": {
      "before": "CONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS=y",
      "after": "# CONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS is not set\nCONFIG_CAMERA_OV5647_MIPI_RAW10_1280x960_BINNING_45FPS=y"
    },
    "available_options": [
      "CONFIG_CAMERA_OV5647_MIPI_RAW8_800x1280_50FPS",
      "CONFIG_CAMERA_OV5647_MIPI_RAW8_800x640_50FPS",
      "CONFIG_CAMERA_OV5647_MIPI_RAW8_800x800_50FPS",
      "CONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS",
      "CONFIG_CAMERA_OV5647_MIPI_RAW10_1280x960_BINNING_45FPS"
    ],
    "selected": "CONFIG_CAMERA_OV5647_MIPI_RAW10_1280x960_BINNING_45FPS",
    "reason": "지원 해상도 중 메모리 사용 최소화 + 충분한 화질"
  },
  "implementation": {
    "step_1": {
      "action": "sdkconfig.defaults 수정",
      "line": "17-19",
      "change": "1920x1080 → 1280x960",
      "comment": "메모리 최적화, 1080p 대비 41% 감소"
    },
    "step_2": {
      "action": "Clean build 필수",
      "command": "idf.py fullclean && idf.py build",
      "reason": "sdkconfig 변경 시 기존 빌드 캐시 무효화 필요"
    },
    "step_3": {
      "action": "Flash 및 테스트",
      "command": "idf.py flash monitor",
      "verify": [
        "로그: app_video: width=1280 height=960",
        "로그: h264_enc: Resolution: 1280x960",
        "메모리 할당 성공 확인"
      ]
    }
  },
  "memory_calculation": {
    "resolution_1920x1080": {
      "yuv420": "1920 × 1080 × 1.5 = 3,110,400 bytes (2.97 MB)",
      "status": "❌ 메모리 부족"
    },
    "resolution_1280x960": {
      "yuv420": "1280 × 960 × 1.5 = 1,843,200 bytes (1.76 MB)",
      "reduction": "41% (1080p 대비)",
      "peak_memory": "~20-25 MB",
      "spiram_available": "32 MB",
      "status": "✅ 충분"
    }
  },
  "lessons_learned": [
    "ESP-IDF는 sdkconfig가 최우선 설정",
    "코드에서 VIDIOC_S_FMT로 해상도 변경 불가 (이미 초기화됨)",
    "카메라 해상도는 반드시 sdkconfig.defaults에서 설정",
    "sdkconfig 변경 시 fullclean 필수",
    "example 폴더가 정답 - 공식 예제 참고 필수"
  ],
  "commits": [
    {
      "hash": "pending",
      "message": "sdkconfig: 카메라 해상도 1280x960 적용 (CONFIG 설정)"
    }
  ],
  "expected_results": {
    "camera_init": "✅ width=1280 height=960",
    "encoder": "✅ Resolution: 1280x960",
    "memory": "✅ 1.76 MB/frame 할당 성공",
    "recording": "✅ 100/100 프레임",
    "fps": "✅ 20.00",
    "file_size": "~2-3 MB",
    "failures": "0건"
  },
  "next_steps": [
    "1. idf.py fullclean",
    "2. idf.py build",
    "3. idf.py flash",
    "4. idf.py monitor",
    "5. 로그 확인: width=1280 height=960",
    "6. 메모리 할당 성공 확인",
    "7. 100 프레임 녹화 확인"
  ],
  "raw_content": {
    "question": "아니;; 왜 안돼는데\n\n체크리스트!! js파일 작성!! 깃반영\n\nexample/video_lcd_display 이게 공식 예제니까 이걸 봐\n\n[로그]\nI (2391) h264_enc: Resolution: 1920x1080  ← 여전히 1080p!",
    "answer": "example/video_lcd_display를 분석한 결과, **sdkconfig.defaults**에서 카메라 해상도를 설정해야 합니다!\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ 문제 원인 발견!\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\napp_video.c에서 EXAMPLE_FORCE_CAM_WIDTH/HEIGHT를 변경해도\nsdkconfig.defaults에 다음 설정이 있어서 무시됨:\n\nCONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS=y  ← 이게 문제!\n\nESP-IDF는 sdkconfig 설정을 우선하여 센서 드라이버 초기화 시\n레지스터 설정을 CONFIG 값으로 선택합니다.\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ 해결: sdkconfig.defaults 수정\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\nsdkconfig.defaults Line 17-19:\n\nBefore:\nCONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS=y\n\nAfter:\n# CONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS is not set\nCONFIG_CAMERA_OV5647_MIPI_RAW10_1280x960_BINNING_45FPS=y\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ 빌드 방법 (IMPORTANT!)\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\nsdkconfig 변경 시 fullclean 필수:\n\n1. idf.py fullclean\n2. idf.py build\n3. idf.py flash\n4. idf.py monitor\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ 예상 결과\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n✅ I app_video: width=1280 height=960\n✅ I h264_enc: Resolution: 1280x960\n✅ 메모리: 1.76 MB/frame\n✅ 100 프레임 모두 성공\n✅ FPS: 20.00\n✅ 파일: ~2-3 MB\n\nrequest(58).js 생성 및 커밋하겠습니다."
  }
};
