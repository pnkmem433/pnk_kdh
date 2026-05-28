// request(6).js
window.requestData = window.requestData || [];
window.requestData[6] = {
  "request_number": 6,
  "title": "빌드 오류 수정 및 작업 방식 확립",
  "date": "2025-10-20",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request"
  ],
  "problem": {
    "summary": "요청 #6 - 빌드 오류 수정 및 작업 방식 확립"
  },
  "solution": {
    "summary": "요청 #6 - 빌드 오류 수정 및 작업 방식 확립"
  },
  "raw_content": {
    "question": "See sections below",
    "answer": "See sections below"
  },
  "sections": {
    "빌드 오류 분석": "【문제】\nuint32_t 타입의 변수를 printf 포맷 지정자 %d로 출력 시도\n\n【발생 위치】\n1. main.c:140 - video_save_progress_cb 함수\n   - percent 변수 (uint32_t)를 %d로 출력\n\n2. main.c:205 - camera_video_frame_operation 함수\n   - progress 변수 (uint32_t)를 %d로 출력\n   - s_mjpeg_frames 변수 (uint32_t)를 %d로 출력\n\n【원인】\n- %d는 signed int를 위한 포맷 지정자\n- uint32_t는 unsigned 32비트 정수\n- GCC의 -Werror=format 플래그로 인해 경고가 오류로 처리됨\n- ESP-IDF는 타입 안정성을 위해 엄격한 컴파일 옵션 사용",
    "해결 방법": "【수정 내용】\nuint32_t 타입을 출력할 때 unsigned long으로 캐스팅하고 %lu 포맷 사용\n\n1. main.c:140 수정 (video_save_progress_cb 함수)\n   수정 전:\n   ```c\n   if (percent >= last_percent + 10) {\n       ESP_LOGI(TAG, \"동영상 녹화 후 저장중 %d%%\", percent);\n       last_percent = percent;\n   }\n   ```\n\n   수정 후:\n   ```c\n   if (percent >= last_percent + 10) {\n       ESP_LOGI(TAG, \"동영상 녹화 후 저장중 %lu%%\", (unsigned long)percent);\n       last_percent = percent;\n   }\n   ```\n\n2. main.c:205 수정 (camera_video_frame_operation 함수)\n   수정 전:\n   ```c\n   if (progress >= last_progress + 10 && progress <= 100) {\n       ESP_LOGI(TAG, \"동영상 녹화중 %d%% (%d/300 프레임)\", progress, s_mjpeg_frames);\n       last_progress = progress;\n   }\n   ```\n\n   수정 후:\n   ```c\n   if (progress >= last_progress + 10 && progress <= 100) {\n       ESP_LOGI(TAG, \"동영상 녹화중 %lu%% (%lu/300 프레임)\", (unsigned long)progress, (unsigned long)s_mjpeg_frames);\n       last_progress = progress;\n   }\n   ```\n\n【이유】\n- %lu는 unsigned long을 위한 포맷 지정자\n- unsigned long으로 캐스팅하여 플랫폼 간 호환성 보장\n- C 표준에서 안전하고 명확한 방법\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n[질문 #2]\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\nesp-ide 환경에서 로딩하려면\n'c:\\Users\\User\\.vscode\\extensions\\espressif.esp-idf-extension-1.10.2\\export.ps1'\n를 먼저 실행해야해\n\n아까랑 지금이랑 합쳐서 txt랑 깃헙 반영해줘\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n[답변 #2] ( Claude )\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━",
    "ESP-IDF 환경 로드 방법": "【VSCode ESP-IDF 확장 환경 로드】\nPowerShell 스크립트 실행:\n```powershell\npowershell -ExecutionPolicy Bypass -File \"c:\\Users\\User\\.vscode\\extensions\\espressif.esp-idf-extension-1.10.2\\export.ps1\"\n```\n\n이 스크립트는 다음을 수행:\n- ESP-IDF 환경 변수 설정\n- Python 가상 환경 활성화\n- idf.py 명령어 경로 추가\n- 크로스 컴파일러 경로 추가\n\n【참고】\n- VSCode의 빌드 작업(Task)은 자동으로 이 환경을 로드함\n- 터미널에서 직접 빌드할 경우 수동으로 실행 필요",
    "수행한 작업": "【체크리스트】\n- [x] main.c의 printf 포맷 오류 수정 (%d → %lu)\n- [x] ESP-IDF 환경 로드 (export.ps1)\n- [x] 빌드 테스트 (사용자가 VSCode에서 실행)\n- [x] 질문 #1과 #2를 합쳐 대화 내용을 request(6).txt에 기록\n- [ ] git add 실행\n- [ ] git commit 실행\n- [ ] git push 실행\n\n【변경된 파일】\n- main/main.c (2개 위치의 포맷 지정자 수정)",
    "참고 사항": "【printf 포맷 지정자 (C99 기준)】\n- %d, %i : signed int\n- %u : unsigned int\n- %ld : signed long\n- %lu : unsigned long\n- %lld : signed long long\n- %llu : unsigned long long\n- PRIu32 : uint32_t를 위한 매크로 (inttypes.h)\n- PRId32 : int32_t를 위한 매크로 (inttypes.h)\n- %zu : size_t (C99 이상)\n\n【ESP-IDF 권장 사항】\nESP-IDF에서는 플랫폼 독립적인 정수 타입 출력을 위해 다음을 권장:\n- uint32_t → PRIu32 매크로 또는 (unsigned long) 캐스팅 + %lu\n- int32_t → PRId32 매크로 또는 (long) 캐스팅 + %ld\n- size_t → %zu (C99 이상)\n- 포인터 → %p\n\n【다른 해결 방법】\n1. PRIu32 매크로 사용:\n   ```c\n   #include <inttypes.h>\n   ESP_LOGI(TAG, \"동영상 녹화중 %\" PRIu32 \"%%\", percent);\n   ```\n\n2. %u 사용 (uint32_t가 unsigned int와 같은 크기일 때):\n   ```c\n   ESP_LOGI(TAG, \"동영상 녹화중 %u%%\", (unsigned)percent);\n   ```\n\n이번 구현에서는 가장 범용적인 방법인 (unsigned long) 캐스팅 + %lu를 사용했습니다.\n\n================================================================="
  }
};
