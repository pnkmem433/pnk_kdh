// request(34).js
window.requestData = window.requestData || [];
window.requestData[34] = {
  "request_number": 34,
  "title": "H.264 파일 저장 실패 문제 해결 - 상세 로그 추가",
  "date": "2025-10-27T18:40:00+09:00",
  "author": "Claude (AI Assistant)",
  "related_requests": [
    33,
    34
  ],
  "status": "completed",
  "tags": [
    "h264",
    "file-save",
    "debugging",
    "errno-22",
    "logging"
  ],
  "problem": {
    "summary": "H.264 인코딩은 성공하지만 최종 파일 저장이 실패하는 문제",
    "error_code": "errno 22 (Invalid argument)",
    "error_location": "h264_spool.c:437 - fopen(output_path, \"wb\")",
    "symptoms": [
      "8개 프레임 인코딩 성공",
      "임시 파일(h264temp.dat) 쓰기 성공",
      "최종 파일(/sdcard/video001.h264) 열기 실패",
      "errno 22: Invalid argument 발생"
    ],
    "logs": [
      "I (24451) h264_spool: SD writer stopped (8 frames written)",
      "E (24452) h264_spool: Failed to open output file: /sdcard/video001.h264 (errno: 22, Invalid argument)"
    ]
  },
  "analysis": {
    "root_cause": "fopen() 호출 시 파일 경로 또는 파일 시스템 상태 문제",
    "possible_reasons": [
      "파일 경로에 특수 문자 또는 잘못된 인코딩",
      "파일이 이미 열려있거나 잠긴 상태",
      "FAT32 파일시스템 제약사항",
      "메모리 부족으로 인한 버퍼 할당 실패",
      "SD카드 쓰기 권한 문제"
    ],
    "investigation_needed": [
      "파일 경로의 각 바이트 확인",
      "디렉토리 존재 여부 확인",
      "기존 파일 존재 시 삭제 시도",
      "다양한 파일명으로 테스트",
      "재시도 로직 추가"
    ]
  },
  "solution": {
    "approach": "상세한 디버깅 로그 추가 및 파일 열기 프로세스 개선",
    "changes": [
      {
        "file": "main/h264_spool.c",
        "function": "h264_spool_flush_to_file",
        "line_range": "434-520",
        "description": "출력 파일 열기 전 상세 검증 로직 추가",
        "details": [
          "파일 경로의 각 문자 16진수 출력",
          "디렉토리 존재 여부 stat() 확인",
          "기존 파일 존재 시 unlink() 시도",
          "메모리 상태 확인 (Internal/SPIRAM)",
          "다양한 파일명으로 테스트 (test.h264, video001.mp4 등)",
          "실패 시 자동 재시도 로직"
        ]
      },
      {
        "file": "main/h264_spool.c",
        "function": "h264_spool_flush_to_file",
        "line_range": "525-599",
        "description": "파일 복사 프로세스 상세 로그 추가",
        "details": [
          "각 프레임 읽기/쓰기 결과 로그",
          "NAL 크기 유효성 검증 (0 < size < 10MB)",
          "fread/fwrite 반환값 검증",
          "첫 3개 프레임과 마지막 프레임 상세 로그",
          "총 바이트 수 추적"
        ]
      },
      {
        "file": "main/h264_spool.c",
        "function": "h264_spool_flush_to_file",
        "line_range": "600-654",
        "description": "파일 닫기 및 검증 로그 추가",
        "details": [
          "fflush() 오류 체크",
          "fclose() 오류 체크",
          "stat()로 최종 파일 크기 검증",
          "기대 크기와 실제 크기 비교",
          "임시 파일 삭제 결과 로그"
        ]
      }
    ]
  },
  "implementation": {
    "code_changes": {
      "added_logging": [
        "파일 경로 문자별 16진수 덤프",
        "디렉토리/파일 stat() 결과",
        "fopen() errno 상세 정보",
        "대체 파일명 테스트 결과",
        "프레임별 read/write 결과",
        "파일 I/O 오류 상세 정보",
        "최종 파일 크기 검증"
      ],
      "added_error_handling": [
        "기존 파일 자동 삭제",
        "파일 열기 재시도 로직",
        "NAL 크기 범위 검증",
        "fread/fwrite 반환값 검증",
        "fflush/fclose 오류 처리"
      ]
    },
    "build_result": "성공 (2025-10-27 18:33)",
    "files_modified": [
      "main/h264_spool.c"
    ]
  },
  "expected_results": {
    "diagnostic_info": [
      "파일 경로의 정확한 바이트 구성 확인 가능",
      "파일 시스템 상태 확인 (디렉토리 존재, 권한 등)",
      "파일 열기 실패의 정확한 원인 파악",
      "어떤 파일명이 성공하는지 확인",
      "재시도로 성공 가능한지 확인"
    ],
    "problem_resolution": [
      "파일 경로 문제 식별 및 수정",
      "또는 파일 시스템 문제 우회 방법 발견",
      "안정적인 파일 저장 성공"
    ]
  },
  "testing": {
    "build_command": "powershell -ExecutionPolicy Bypass -File build_ov5647.ps1",
    "build_time": "2025-10-27 18:33",
    "build_status": "성공",
    "next_steps": [
      "ESP32-P4 보드에 플래시",
      "모니터로 로그 확인",
      "파일 경로 문제 진단",
      "필요시 추가 수정"
    ]
  },
  "technical_details": {
    "file_system": "FAT32 (SD card)",
    "platform": "ESP32-P4 (ESP-IDF v5.4.1)",
    "error_code": "EINVAL (22)",
    "affected_function": "fopen()",
    "temp_file": "/sdcard/h264temp.dat (성공)",
    "output_file": "/sdcard/video001.h264 (실패)",
    "frames_encoded": 8,
    "encoder_status": "정상 (첫 프레임 제외)"
  },
  "related_issues": {
    "request_32": "포맷 변환 문제 해결",
    "request_33": "U/V 인덱싱 버그 수정",
    "request_34": "HW 인코더 특수 메모리 레이아웃 적용"
  },
  "additional_changes": {
    "request_format": "TXT → JSON 변환 제안",
    "reason": "웹에서 쉽게 볼 수 있도록 JSON 형식 사용",
    "structure": {
      "metadata": "request_number, title, date, author, related_requests, status, tags",
      "content": "problem, analysis, solution, implementation, expected_results",
      "technical": "technical_details, testing, related_issues"
    }
  },
  "checklist": [
    {
      "task": "문제 분석 및 근본 원인 파악",
      "status": "completed"
    },
    {
      "task": "h264_spool.c 파일 읽기 및 이해",
      "status": "completed"
    },
    {
      "task": "상세 로그 추가 (파일 열기)",
      "status": "completed"
    },
    {
      "task": "상세 로그 추가 (파일 복사)",
      "status": "completed"
    },
    {
      "task": "상세 로그 추가 (파일 검증)",
      "status": "completed"
    },
    {
      "task": "프로젝트 빌드",
      "status": "completed"
    },
    {
      "task": "request(35).json 파일 작성",
      "status": "completed"
    },
    {
      "task": "Git 커밋",
      "status": "pending"
    }
  ],
  "notes": [
    "이번 요청부터 request 파일을 JSON 형식으로 저장",
    "JSON 형식은 웹 기반 뷰어에서 쉽게 파싱 가능",
    "구조화된 데이터로 검색 및 필터링 용이",
    "이전 TXT 파일들도 필요시 JSON으로 변환 가능"
  ]
};
