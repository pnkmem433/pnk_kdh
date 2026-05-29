// request(3).js
window.requestData = window.requestData || [];
window.requestData[3] = {
  "request_number": 3,
  "title": "빌드 오류 해결 (경로 공백 문제)",
  "date": "2025-10-20",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request"
  ],
  "problem": {
    "summary": "빌드 시 다음 오류 발생:"
  },
  "solution": {
    "summary": "Python 스크립트(esp_ipa_config.py)가 경로를 파싱할 때 공백을 잘못 처리"
  },
  "raw_content": {
    "question": "빌드 시 다음 오류 발생:\n\n```\n[853/1053] Generating  IPA configuration file...\nFAILED: esp-idf/espressif__esp_ipa/esp_video_ipa_config.c\nFileNotFoundError: [Errno 2] No such file or directory: 'D:/04.'\n```\n\n오류 해결 필요",
    "answer": "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n─────────────────────────────────────────────────────────────────\n■ 오류 원인 분석\n─────────────────────────────────────────────────────────────────\n\n【문제】\nPython 스크립트(esp_ipa_config.py)가 경로를 파싱할 때 공백을 잘못 처리\n\n현재 경로: `D:/04. pretests-iot/251020_pnklabmem203_esp32p4_switchbox_PoC`\n인식된 경로: `D:/04.` ← 첫 번째 공백에서 잘림\n\n【원인】\n- 경로에 공백이 포함됨: \"04. pretests-iot\"\n- ESP-IDF의 esp_ipa 컴포넌트 빌드 스크립트가 공백을 포함한 경로를\n  제대로 처리하지 못함\n- CMake에서 경로를 전달할 때 따옴표 처리가 누락됨\n\n─────────────────────────────────────────────────────────────────\n■ 해결 방법\n─────────────────────────────────────────────────────────────────\n\n【방법 1: 프로젝트 경로 변경 (권장)】\n\n프로젝트를 공백이 없는 경로로 이동:\n\n현재 경로:\n`D:\\04. pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC`\n\n권장 경로:\n`D:\\04_pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC`\n또는\n`D:\\pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC`\n\n【단계】\n1. 프로젝트 폴더 이름 변경:\n   `04. pretests-iot` → `04_pretests-iot`\n\n2. build 폴더 삭제:\n   ```\n   rmdir /s /q \"D:\\04_pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC\\build\"\n   ```\n\n3. 빌드 재실행:\n   ```\n   cd \"D:\\04_pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC\"\n   idf.py build\n   ```\n\n─────────────────────────────────────────────────────────────────\n\n【방법 2: sdkconfig에서 카메라 센서 설정 변경】\n\nSC2336 센서를 사용하지 않거나, IPA를 비활성화:\n\n1. `idf.py menuconfig` 실행\n2. `Component config` → `ESP Video` → ISP Pipeline Controller 비활성화\n3. 또는 다른 카메라 센서 선택\n\n하지만 이 방법은 프로젝트 요구사항(SC2336 카메라 사용)을 충족하지 못합니다.\n\n─────────────────────────────────────────────────────────────────\n■ 권장 조치\n─────────────────────────────────────────────────────────────────\n\n**방법 1을 권장합니다.**\n\nESP-IDF는 경로에 공백이 있으면 다양한 문제가 발생할 수 있으므로,\n공백 없는 경로를 사용하는 것이 best practice입니다.\n\n【조치 방법】\n1. 현재 프로젝트 폴더 닫기\n2. Windows 탐색기에서 폴더 이름 변경:\n   `04. pretests-iot` → `04_pretests-iot`\n3. VSCode에서 새 경로로 프로젝트 다시 열기\n4. build 폴더 삭제 후 재빌드\n\n─────────────────────────────────────────────────────────────────\n■ 참고 사항\n─────────────────────────────────────────────────────────────────\n\nESP-IDF 공식 문서에서도 권장하는 사항:\n- 프로젝트 경로에 공백, 특수문자 사용 금지\n- ASCII 문자만 사용\n- 한글 경로 사용 금지\n\n이는 크로스 플랫폼 빌드 시스템(CMake, Ninja)과의 호환성을 위한 것입니다.\n\n================================================================="
  },
  "sections": {
    "오류 원인 분석": "【문제】\nPython 스크립트(esp_ipa_config.py)가 경로를 파싱할 때 공백을 잘못 처리\n\n현재 경로: `D:/04. pretests-iot/251020_pnklabmem203_esp32p4_switchbox_PoC`\n인식된 경로: `D:/04.` ← 첫 번째 공백에서 잘림\n\n【원인】\n- 경로에 공백이 포함됨: \"04. pretests-iot\"\n- ESP-IDF의 esp_ipa 컴포넌트 빌드 스크립트가 공백을 포함한 경로를\n  제대로 처리하지 못함\n- CMake에서 경로를 전달할 때 따옴표 처리가 누락됨",
    "해결 방법": "【방법 1: 프로젝트 경로 변경 (권장)】\n\n프로젝트를 공백이 없는 경로로 이동:\n\n현재 경로:\n`D:\\04. pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC`\n\n권장 경로:\n`D:\\04_pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC`\n또는\n`D:\\pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC`\n\n【단계】\n1. 프로젝트 폴더 이름 변경:\n   `04. pretests-iot` → `04_pretests-iot`\n\n2. build 폴더 삭제:\n   ```\n   rmdir /s /q \"D:\\04_pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC\\build\"\n   ```\n\n3. 빌드 재실행:\n   ```\n   cd \"D:\\04_pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC\"\n   idf.py build\n   ```\n\n─────────────────────────────────────────────────────────────────\n\n【방법 2: sdkconfig에서 카메라 센서 설정 변경】\n\nSC2336 센서를 사용하지 않거나, IPA를 비활성화:\n\n1. `idf.py menuconfig` 실행\n2. `Component config` → `ESP Video` → ISP Pipeline Controller 비활성화\n3. 또는 다른 카메라 센서 선택\n\n하지만 이 방법은 프로젝트 요구사항(SC2336 카메라 사용)을 충족하지 못합니다.",
    "권장 조치": "**방법 1을 권장합니다.**\n\nESP-IDF는 경로에 공백이 있으면 다양한 문제가 발생할 수 있으므로,\n공백 없는 경로를 사용하는 것이 best practice입니다.\n\n【조치 방법】\n1. 현재 프로젝트 폴더 닫기\n2. Windows 탐색기에서 폴더 이름 변경:\n   `04. pretests-iot` → `04_pretests-iot`\n3. VSCode에서 새 경로로 프로젝트 다시 열기\n4. build 폴더 삭제 후 재빌드",
    "참고 사항": "ESP-IDF 공식 문서에서도 권장하는 사항:\n- 프로젝트 경로에 공백, 특수문자 사용 금지\n- ASCII 문자만 사용\n- 한글 경로 사용 금지\n\n이는 크로스 플랫폼 빌드 시스템(CMake, Ninja)과의 호환성을 위한 것입니다.\n\n================================================================="
  }
};
