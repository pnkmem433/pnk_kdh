// request(38).js
window.requestData = window.requestData || [];
window.requestData[38] = {
  "request_number": 38,
  "title": "컴파일 오류 수정: snprintf 버퍼 오버플로우 경고",
  "date": "2025-10-28",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "bug-fix",
    "compilation-error",
    "buffer-overflow",
    "security"
  ],
  "problem": {
    "summary": "h264_multi_encoder.c 컴파일 시 snprintf 버퍼 크기 부족 경고로 빌드 실패",
    "error_details": {
      "file": "main/h264_multi_encoder.c",
      "line": 283,
      "error_type": "format-truncation warning (treated as error)",
      "compiler_flag": "-Werror=format-truncation",
      "error_message": "'%u' directive output may be truncated writing between 1 and 10 bytes into a region of size 7"
    },
    "root_cause": {
      "code": "char task_name[16];\nsnprintf(task_name, sizeof(task_name), \"h264_enc_%u\", (unsigned)i);",
      "issue": "버퍼 크기 16바이트가 최대 출력 20바이트보다 작음",
      "calculation": {
        "format_string": "h264_enc_%u",
        "fixed_part": "h264_enc_",
        "fixed_part_length": 9,
        "variable_part": "%u (unsigned int)",
        "variable_max_length": 10,
        "null_terminator": 1,
        "total_max": "9 + 10 + 1 = 20 bytes",
        "buffer_size": "16 bytes",
        "shortage": "4 bytes"
      }
    },
    "compiler_behavior": {
      "warning_level": "-Werror",
      "effect": "모든 경고를 에러로 처리",
      "reason": "잠재적 버퍼 오버플로우 방지 (보안 취약점)"
    }
  },
  "solution": {
    "summary": "task_name 버퍼 크기를 16에서 24로 증가",
    "approach": "충분한 여유를 가진 버퍼 크기로 변경하여 컴파일러 경고 제거",
    "code_change": {
      "file": "main/h264_multi_encoder.c",
      "line": 282,
      "before": "char task_name[16];",
      "after": "char task_name[24];  // 버퍼 크기 증가: 16 → 24 (충분한 여유)",
      "rationale": [
        "최대 필요 크기: 20바이트",
        "새 버퍼 크기: 24바이트",
        "여유 공간: 4바이트",
        "안전 마진 확보"
      ]
    },
    "verification": {
      "max_encoder_id": "실제 사용: 0-2 (3개 인코더)",
      "max_string_length": "\"h264_enc_2\" = 11바이트 (실제)",
      "worst_case": "\"h264_enc_4294967295\" = 20바이트 (이론상 최대)",
      "buffer_utilization": "20 / 24 = 83.3% (안전)",
      "remaining_space": "4바이트"
    }
  },
  "raw_content": {
    "question": "[컴파일 에러 로그]\nD:/04.pretests-iot/251020_pnklabmem203_esp32p4_switchbox_PoC/main/h264_multi_encoder.c: In function 'h264_multi_encoder_init':\nD:/04.pretests-iot/251020_pnklabmem203_esp32p4_switchbox_PoC/main/h264_multi_encoder.c:283:58: error: '%u' directive output may be truncated writing between 1 and 10 bytes into a region of size 7 [-Werror=format-truncation=]\n  283 |         snprintf(task_name, sizeof(task_name), \"h264_enc_%u\", (unsigned)i);\n      |                                                          ^~\nD:/04.pretests-iot/251020_pnklabmem203_esp32p4_switchbox_PoC/main/h264_multi_encoder.c:283:48: note: directive argument in the range [0, 4294967294]\n  283 |         snprintf(task_name, sizeof(task_name), \"h264_enc_%u\", (unsigned)i);\n      |                                                ^~~~~~~~~~~~~\nD:/04.pretests-iot/251020_pnklabmem203_esp32p4_switchbox_PoC/main/h264_multi_encoder.c:283:9: note: 'snprintf' output between 11 and 20 bytes into a destination of size 16\n  283 |         snprintf(task_name, sizeof(task_name), \"h264_enc_%u\", (unsigned)i);\n      |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\ncc1.exe: some warnings being treated as errors\n[5/11] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/main.c.obj\nninja: build stopped: subcommand failed.\n\nThe terminal process \"C:\\Espressif\\tools\\ninja\\1.12.1\\ninja.EXE\" terminated with exit code: 1.\n\n컴파일 에러를 해결하라 Ai/request/request(38).js에 작성",
    "answer": "## 에러 분석\n\n### 컴파일 오류 메시지 해석\n\n```\nerror: '%u' directive output may be truncated writing between 1 and 10 bytes \ninto a region of size 7 [-Werror=format-truncation=]\n```\n\n**문제점**:\n1. `char task_name[16]` - 버퍼 크기 16바이트\n2. `snprintf(task_name, 16, \"h264_enc_%u\", i)` - 최대 20바이트 필요\n3. 버퍼 오버플로우 가능성 → 컴파일러 경고 → `-Werror`로 에러 처리\n\n### 버퍼 크기 계산\n\n**포맷 문자열**: `\"h264_enc_%u\"`\n\n| 구성 요소 | 길이 | 설명 |\n|----------|------|------|\n| \"h264_enc_\" | 9 bytes | 고정 문자열 |\n| %u (최대값) | 10 bytes | unsigned int 최대값: 4,294,967,295 (10자리) |\n| null terminator | 1 byte | 문자열 종료 |\n| **총합** | **20 bytes** | 최대 필요 크기 |\n\n**현재 버퍼**: 16 bytes → **4 bytes 부족!**\n\n### 실제 vs 이론상 크기\n\n**실제 사용** (3개 인코더):\n- 인코더 0: \"h264_enc_0\" = 11 bytes\n- 인코더 1: \"h264_enc_1\" = 11 bytes\n- 인코더 2: \"h264_enc_2\" = 11 bytes\n\n**이론상 최대** (unsigned int):\n- 인코더 4294967295: \"h264_enc_4294967295\" = 20 bytes\n\n컴파일러는 **이론상 최대값**을 기준으로 경고를 발생시킵니다.\n\n### 컴파일러 동작 원리\n\n**GCC -Werror=format-truncation**:\n- snprintf의 출력이 버퍼 크기를 초과할 가능성 검사\n- 타입의 최대값 기준으로 계산 (unsigned int → 10자리)\n- 잠재적 버퍼 오버플로우 방지 (보안 취약점)\n\n**ESP-IDF 빌드 설정**:\n- `-Werror` 플래그: 모든 경고를 에러로 처리\n- 안전한 코드 작성 강제\n- 런타임 오류 사전 방지\n\n## 해결 방법\n\n### 수정 내용\n\n```c\n// 수정 전\nchar task_name[16];\nsnprintf(task_name, sizeof(task_name), \"h264_enc_%u\", (unsigned)i);\n\n// 수정 후\nchar task_name[24];  // 버퍼 크기 증가: 16 → 24 (충분한 여유)\nsnprintf(task_name, sizeof(task_name), \"h264_enc_%u\", (unsigned)i);\n```\n\n**변경 사항**:\n- 파일: [main/h264_multi_encoder.c:282](main/h264_multi_encoder.c#L282)\n- 버퍼 크기: 16 → 24 (50% 증가)\n\n### 버퍼 크기 선택 근거\n\n**24 bytes 선택 이유**:\n1. 최대 필요 크기: 20 bytes\n2. 여유 공간: 4 bytes (20% 마진)\n3. 메모리 정렬: 8의 배수 (스택 효율)\n4. 가독성: 명확한 여유 공간\n\n**대안 크기들**:\n- 20 bytes: 최소 필요 (여유 0, 위험)\n- 21-23 bytes: 여유 부족\n- **24 bytes**: 적절한 여유 ✓\n- 32 bytes: 과도한 여유 (낭비)\n\n### 검증\n\n**실제 사용량**:\n```\n인코더 0: \"h264_enc_0\" = 11 / 24 = 45.8%\n인코더 1: \"h264_enc_1\" = 11 / 24 = 45.8%\n인코더 2: \"h264_enc_2\" = 11 / 24 = 45.8%\n```\n\n**이론상 최대**:\n```\n최대값: \"h264_enc_4294967295\" = 20 / 24 = 83.3%\n여유: 4 bytes (안전)\n```\n\n## 추가 정보\n\n### 버퍼 오버플로우 취약점\n\n**보안 위험**:\n- 스택 버퍼 오버플로우 → 임의 코드 실행\n- Return address 덮어쓰기 → 제어 흐름 변조\n- 임베디드 시스템에서 치명적\n\n**방지 기법**:\n1. **컴파일 타임**: `-Werror=format-truncation` (이번 케이스)\n2. **런타임**: Stack canary, ASLR\n3. **코딩 규칙**: snprintf 대신 안전한 함수 사용\n\n### snprintf vs sprintf\n\n**sprintf (위험)**:\n```c\nsprintf(task_name, \"h264_enc_%u\", i);  // 버퍼 크기 체크 없음!\n```\n\n**snprintf (안전)**:\n```c\nsnprintf(task_name, sizeof(task_name), \"h264_enc_%u\", i);  // 자동 잘림\n```\n\n**차이점**:\n- `sprintf`: 버퍼 크기 무시 → 오버플로우 위험\n- `snprintf`: 버퍼 크기 제한 → 자동 잘림 (안전)\n- 항상 `snprintf` 사용 권장\n\n### 메모리 영향\n\n**스택 사용량**:\n- 수정 전: 16 bytes × 1회 = 16 bytes\n- 수정 후: 24 bytes × 1회 = 24 bytes\n- 증가량: 8 bytes (무시할 수 있는 수준)\n\n**FreeRTOS 태스크 스택**:\n- 워커 태스크 스택: 8192 bytes (8KB)\n- 증가량: 8 bytes / 8192 bytes = 0.098%\n- 영향: 무시할 수 있음\n\n## 결론\n\n**문제**: snprintf 버퍼 크기 부족 (16 bytes < 20 bytes 필요)\n\n**해결**: 버퍼 크기를 24 bytes로 증가\n\n**효과**:\n- ✓ 컴파일 오류 해결\n- ✓ 버퍼 오버플로우 방지\n- ✓ 충분한 안전 마진 확보 (4 bytes)\n- ✓ 메모리 낭비 최소화 (8 bytes 증가)\n\n**테스트**:\n```bash\nidf.py build  # 빌드 성공 확인\n```\n\n수정 완료!"
  },
  "sections": {
    "에러_원인": {
      "버퍼_크기": "16 bytes",
      "필요_크기": "20 bytes (최대)",
      "부족량": "4 bytes",
      "컴파일러_동작": "이론상 최대값 기준 검사 (unsigned int: 10자리)"
    },
    "버퍼_크기_계산": {
      "고정_문자열": "\"h264_enc_\" = 9 bytes",
      "변수_부분": "%u 최대 = 10 bytes",
      "null_terminator": "1 byte",
      "총합": "20 bytes"
    },
    "해결_방법": {
      "수정": "char task_name[16] → char task_name[24]",
      "파일": "main/h264_multi_encoder.c:282",
      "여유_공간": "4 bytes (20% 마진)"
    },
    "검증": {
      "실제_사용": "11 bytes (\"h264_enc_2\")",
      "이론상_최대": "20 bytes",
      "버퍼_활용률": "83.3% (20/24)",
      "안전성": "✓"
    },
    "메모리_영향": {
      "증가량": "8 bytes",
      "태스크_스택_비율": "0.098% (8/8192)",
      "평가": "무시할 수 있는 수준"
    }
  },
  "technical_details": {
    "compiler_warning": {
      "flag": "-Werror=format-truncation",
      "description": "snprintf 출력이 버퍼 크기 초과 가능성 검사",
      "action": "경고를 에러로 처리 → 빌드 실패"
    },
    "security_implication": {
      "vulnerability": "Stack buffer overflow",
      "risk": "임의 코드 실행, 제어 흐름 변조",
      "mitigation": "컴파일 타임 검사 + 충분한 버퍼 크기"
    },
    "best_practices": {
      "rule_1": "항상 snprintf 사용 (sprintf 금지)",
      "rule_2": "타입의 최대값 고려하여 버퍼 크기 결정",
      "rule_3": "안전 마진 확보 (최소 10-20%)",
      "rule_4": "-Werror 플래그로 경고를 에러로 처리"
    }
  },
  "code_changes": [
    {
      "file": "main/h264_multi_encoder.c",
      "line": 282,
      "change": "버퍼 크기 증가",
      "before": "char task_name[16];",
      "after": "char task_name[24];  // 버퍼 크기 증가: 16 → 24 (충분한 여유)"
    }
  ],
  "verification": {
    "build_command": "idf.py build",
    "expected_result": "빌드 성공 (경고/에러 없음)",
    "memory_impact": "스택 8 bytes 증가 (무시할 수 있음)"
  }
};
