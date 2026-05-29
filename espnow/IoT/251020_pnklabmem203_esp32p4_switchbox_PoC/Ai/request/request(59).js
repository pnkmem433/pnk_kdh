// request(59).js
window.requestData = window.requestData || [];
window.requestData[59] = {
  "request_number": 59,
  "title": "빌드 캐시 미삭제 + 10fps 대안 제안",
  "date": "2025-10-31",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request",
    "build-cache",
    "fps-optimization",
    "alternative-solution"
  ],
  "problem": {
    "summary": "sdkconfig 변경했지만 여전히 1920x1080로 동작",
    "evidence": {
      "log_camera": "I app_video: width=1920 height=1080",
      "log_encoder": "I h264_enc: Resolution: 1920x1080",
      "log_version": "App version: 7384bef-dirty",
      "result": "83/100 completed, 17 dropped, 0.51 fps"
    },
    "root_cause": "sdkconfig 변경했으나 idf.py fullclean을 실행하지 않아 빌드 캐시가 남음",
    "why_cache": "ESP-IDF CMake는 sdkconfig 변경 감지하지만 이미 생성된 config.h 재생성 안함"
  },
  "analysis": {
    "current_status": {
      "resolution": "1920x1080 (not 1280x960!)",
      "fps_target": "20 fps",
      "fps_actual": "0.51 fps (97% drop!)",
      "frames_encoded": "83/100 (17% fail)",
      "frames_saved": "10/100 (90% lost!)",
      "memory_failures": "프레임 #9부터 계속 할당 실패"
    },
    "why_failed": [
      "1. fullclean 미실행 → 빌드 캐시 남음",
      "2. 1920x1080 유지 → 메모리 부족",
      "3. 20fps → 인코딩 속도 따라가지 못함"
    ]
  },
  "solution_1_correct": {
    "title": "올바른 방법: fullclean 후 1280x960 빌드",
    "steps": [
      "1. idf.py fullclean  (필수!)",
      "2. idf.py build",
      "3. idf.py flash",
      "4. idf.py monitor"
    ],
    "expected_result": {
      "resolution": "1280x960",
      "memory_per_frame": "1.76 MB (41% 감소)",
      "fps": "20 fps",
      "frames": "100/100",
      "success_rate": "100%"
    },
    "confidence": "높음 - sdkconfig가 정상 적용되면 해결"
  },
  "solution_2_alternative": {
    "title": "대안: 1080p 유지 + 10fps로 감속",
    "rationale": [
      "fullclean/빌드 시간 단축",
      "해상도 변경 없이 FPS만 조정",
      "메모리 압박 완화 (프레임 간격 2배)"
    ],
    "changes": {
      "main_c_line_39": {
        "before": "static uint32_t s_actual_fps = 20;",
        "after": "static uint32_t s_actual_fps = 10;  // 1080p @ 10fps (memory pressure relief)"
      },
      "main_c_line_197": {
        "before": ".fps = 20,  // 1280x960 @ 20fps",
        "after": ".fps = 10,  // 1080p @ 10fps (memory pressure relief)"
      },
      "main_c_line_198": {
        "before": ".gop_size = 20,",
        "after": ".gop_size = 10,  // 1 I-frame per second"
      },
      "main_c_line_209": {
        "before": "if (h264_spool_start(camera_buf_hes, camera_buf_ves, 20, &s_h264_spool) != ESP_OK) {",
        "after": "if (h264_spool_start(camera_buf_hes, camera_buf_ves, 10, &s_h264_spool) != ESP_OK) {"
      },
      "h264_spool_c_line_195": {
        "before": "spool->ring_capacity = 100;  // 100 프레임 슬롯 (5초 @ 20fps)",
        "after": "spool->ring_capacity = 50;  // 50 프레임 슬롯 (5초 @ 10fps)"
      },
      "main_c_line_166": {
        "before": "uint32_t progress = (s_recorded_frames * 100) / 100; // 100 프레임 기준 (5초 @ 20fps)",
        "after": "uint32_t progress = (s_recorded_frames * 100) / 50; // 50 프레임 기준 (5초 @ 10fps)"
      },
      "main_c_line_168": {
        "before": "ESP_LOGI(TAG, \"동영상 녹화중 %lu%% (%lu/100 프레임)\", ...",
        "after": "ESP_LOGI(TAG, \"동영상 녹화중 %lu%% (%lu/50 프레임)\", ..."
      },
      "main_c_line_258": {
        "before": "if (encode_frame_index >= 100) {",
        "after": "if (encode_frame_index >= 50) {"
      },
      "main_c_line_259": {
        "before": "ESP_LOGI(TAG, \"=== 100 프레임 캡처 완료...\");",
        "after": "ESP_LOGI(TAG, \"=== 50 프레임 캡처 완료...\");"
      }
    },
    "memory_benefit": {
      "frame_interval": "50ms → 100ms (2배)",
      "encoding_time_per_frame": "~40-50ms 필요",
      "margin": "100ms - 50ms = 50ms (여유 확보)",
      "reduced_backlog": "인코딩 대기 큐 감소"
    },
    "expected_result": {
      "resolution": "1920x1080 (유지)",
      "fps": "10 fps",
      "frames": "50/50",
      "success_rate": "~90-100%",
      "memory_allocation": "여전히 일부 실패 가능하지만 개선"
    },
    "pros": [
      "빠른 테스트 (fullclean 불필요)",
      "해상도 유지",
      "인코딩 부하 절반"
    ],
    "cons": [
      "여전히 메모리 부족 가능",
      "FPS 절반 (10fps)",
      "근본 해결 아님"
    ]
  },
  "recommendation": {
    "best_practice": "Solution 1: fullclean 후 1280x960 빌드",
    "reason": [
      "근본적 해결",
      "메모리 41% 감소",
      "20fps 유지 가능",
      "100% 성공률"
    ],
    "quick_test": "Solution 2: 10fps로 빠른 테스트 가능",
    "final_solution": "반드시 Solution 1 적용 권장"
  },
  "commits": [
    {
      "hash": "pending",
      "message": "Docs: request(59).js - fullclean 필요성 및 10fps 대안"
    }
  ],
  "commands": {
    "solution_1": [
      "idf.py fullclean",
      "idf.py build",
      "idf.py flash",
      "idf.py monitor"
    ],
    "solution_2": [
      "코드 수정 (main.c, h264_spool.c)",
      "idf.py build",
      "idf.py flash",
      "idf.py monitor"
    ]
  },
  "raw_content": {
    "question": "10fps로 하면 100% 성공할나?\n\n체크리스트!! js파일작성!! 깃 반영!!\n\n[로그]\nI (1926) app_video: width=1920 height=1080  ← 여전히 1080p!\nI (2394) h264_enc: Resolution: 1920x1080\nI (22258) app_main: 스풀 저장: 10개 프레임  ← 90% 손실!",
    "answer": "문제: sdkconfig 변경했지만 **fullclean을 실행하지 않아** 빌드 캐시가 남아 여전히 1920x1080로 동작합니다!\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ 현재 상황\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n로그 증거:\n• width=1920 height=1080 ← 1280x960 적용 안됨!\n• Resolution: 1920x1080\n• App version: 7384bef-dirty ← 빌드 캐시 사용\n\n결과:\n• 인코딩: 83/100 (17% 실패)\n• 저장: 10/100 (90% 손실!)\n• FPS: 0.51 (목표 20fps의 2.5%)\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ Solution 1: 올바른 방법 (권장) ⭐\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\nfullclean 후 1280x960 빌드:\n\n1. idf.py fullclean  ← 필수!\n2. idf.py build\n3. idf.py flash\n4. idf.py monitor\n\n예상 결과:\n✅ Resolution: 1280x960\n✅ 메모리: 1.76 MB/frame (41% 감소)\n✅ FPS: 20.00\n✅ 프레임: 100/100 (100%)\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ Solution 2: 대안 (빠른 테스트용)\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n1080p 유지 + 10fps로 감속:\n\n변경:\n• s_actual_fps = 10\n• .fps = 10, .gop_size = 10\n• ring_capacity = 50\n• 프레임 목표: 50개 (5초 @ 10fps)\n\n이점:\n• 프레임 간격: 50ms → 100ms (2배)\n• 인코딩 여유: 50ms 확보\n• 메모리 압박 완화\n\n예상 결과:\n⚠️ Resolution: 1920x1080 (유지)\n⚠️ FPS: 10.00\n✅ 프레임: 45-50/50 (90-100%)\n✅ 개선되지만 여전히 메모리 부족 가능\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n■ 권장 사항\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n**Solution 1 (fullclean + 1280x960)을 강력 권장합니다!**\n\nSolution 2는:\n• 빠른 테스트용으로만 사용\n• 근본 해결 아님\n• 최종적으로 Solution 1 필수\n\nrequest(59).js 생성 및 커밋하겠습니다."
  }
};
