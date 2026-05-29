// request(10).js
window.requestData = window.requestData || [];
window.requestData[10] = {
  "request_number": 10,
  "title": "ESP32-P4 카메라 I2C 핀 설정 오류 수정",
  "date": "2025-10-20",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request"
  ],
  "problem": {
    "summary": "사용자가 제공한 실행 로그:"
  },
  "solution": {
    "summary": "### 2.1 회로도 분석 (example/guide/회로도.pdf)"
  },
  "raw_content": {
    "question": "문제 상황\n\n사용자가 제공한 실행 로그:\n```\nI (1834) app_main: I2C 핀 설정: SCL=GPIO8, SDA=GPIO7\nE (1840) i2c.master: I2C transaction unexpected nack detected\nE (1844) i2c.master: s_i2c_synchronous_transaction(924): I2C transaction failed\nE (1851) i2c.master: i2c_master_transmit_receive(1220): I2C transaction failed\nE (1858) sccb_i2c: s_sccb_i2c_transmit_receive_reg_a16v8(116): faled to transmit receive\nE (1866) sc2336: Get sensor ID failed\nE (1869) esp_video_init: failed to detect MIPI-CSI camera sensor with address=30\nE (1892) app_video: 카메라 장치 열기 실패: errno=2 (No such file or directory)\n```\n\n**핵심 문제**: I2C 통신 실패로 인해 카메라 센서 초기화가 되지 않음\n\n",
    "answer": "원인 분석\n\n### 2.1 회로도 분석 (example/guide/회로도.pdf)\n\n회로도 2페이지(ESP32-C5 섹션)를 확인한 결과:\n- **CAM_IO0 = GPIO6** (I2C SCL)\n- **CAM_IO1 = GPIO7** (I2C SDA)\n\nJ10 MIPI CSI 커넥터(22핀 FPC)의 핀 배치에서 카메라 I2C 핀 확인됨.\n\n### 2.2 설정 파일 검토\n\n**sdkconfig.defaults** (올바름):\n```\n# Camera I2C pins (from schematic: CAM_IO0=GPIO6 SCL, CAM_IO1=GPIO7 SDA)\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=6\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN=7\n```\n\n**sdkconfig** (문제 있음):\n```\n700: CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=8  ← 잘못된 값!\n701: CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN=7\n```\n\n### 2.3 문제 요약\n\n- sdkconfig.defaults는 GPIO6으로 올바르게 설정되어 있음\n- 그러나 **실제 빌드에 사용되는 sdkconfig 파일**이 GPIO8로 잘못 설정됨\n- 이로 인해 I2C SCL이 잘못된 핀에 연결되어 카메라 센서와 통신 불가\n\n\n\n해결 방법\n\n### 3.1 sdkconfig 파일 수정\n\nsdkconfig 파일 700번 라인 수정:\n```bash\n# 수정 전:\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=8\n\n# 수정 후:\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=6\n```\n\n### 3.2 수정 명령어\n\n```bash\ncd d:\\04.pretests-iot\\251020_pnklabmem203_esp32p4_switchbox_PoC\nsed -i '700s/CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=8/CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=6/' sdkconfig\n```\n\n### 3.3 검증\n\n수정 후 확인:\n```bash\ngrep CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN sdkconfig\n# 출력: CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=6  ← 올바름!\n```\n\n\n\n기대 효과\n\nI2C 핀이 올바르게 설정되면:\n\n1. **I2C 통신 성공**: 카메라 센서(SC2336)와 정상적으로 통신 가능\n2. **센서 ID 읽기 성공**: \"Get sensor ID failed\" 오류 해결\n3. **카메라 초기화 성공**: `/dev/video0` 디바이스 생성됨\n4. **프레임 캡쳐 시작**: 비디오 스트림 정상 작동\n\n\n\n하드웨어 확인 사항\n\n펌웨어 업로드 후 다음 사항을 확인하세요:\n\n### 5.1 MIPI-CSI 카메라 연결\n- [ ] J10 커넥터에 22핀 FPC 케이블로 카메라 연결\n- [ ] FPC 케이블 방향 확인 (접점이 보드 쪽을 향함)\n- [ ] 카메라 전원(VCC_3V3) 정상 공급 확인\n\n### 5.2 I2C 핀 연결 (회로도 기준)\n- [ ] CAM_IO0(GPIO6) → I2C SCL\n- [ ] CAM_IO1(GPIO7) → I2C SDA\n- [ ] Pull-up 저항 확인 (보드 내장)\n\n### 5.3 카메라 센서\n- [ ] SC2336 센서 사용 (1920x1080@30fps)\n- [ ] I2C 주소: 0x30 (7-bit)\n\n\n\n예상 로그 출력\n\n수정 후 정상 작동 시 예상되는 로그:\n```\nI (xxxx) app_main: I2C 핀 설정: SCL=GPIO6, SDA=GPIO7\nI (xxxx) app_main: 카메라 드라이버 초기화 완료\nI (xxxx) app_video: 카메라 장치 열기 시도: /dev/video0\nI (xxxx) app_video: 카메라 장치 열기 성공 (fd=X)\nI (xxxx) app_video: ===  카메라 장치 정보 ===\nI (xxxx) app_video:   driver:  sc2336\nI (xxxx) TAG: 첫 프레임 캡쳐 성공!\n```\n\n\n\n변경된 파일\n\n```\n수정됨: sdkconfig\n  - CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN: 8 → 6\n```\n\n\n\n참고 문서\n\n- 회로도: example/guide/회로도.pdf (Page 2)\n- 개발 보드 가이드: example/guide/가이드.pdf\n- WT99P4C5-S1 Development Board Guide v1.2\n- 핀 배치: J10 MIPI CSI (22-pin FPC, 0.5mm pitch)\n\n\n\n관련 칩 사양\n\n- **ESP32-P4**: 듀얼 코어 360MHz RISC-V, 32MB PSRAM\n- **카메라 센서**: SC2336, 1920x1080@30fps, RAW8 포맷\n- **I2C 인터페이스**: SCCB (Serial Camera Control Bus)\n- **통신 속도**: 100kHz (표준) / 400kHz (고속)\n\n\n\n다음 단계\n\n1. **펌웨어 플래시**: 수정된 설정으로 재빌드 및 업로드\n2. **하드웨어 점검**: 위의 체크리스트 확인\n3. **로그 모니터링**: 시리얼 터미널에서 초기화 과정 확인\n4. **프레임 캡쳐 테스트**: 10초 비디오 녹화 확인\n5. **SD 카드 확인**: 저장된 비디오 파일 재생\n\n\n\n문제 해결 히스토리\n\n### 이전 세션에서 수정한 항목:\n1. ✅ printf 포맷 지정자 수정 (%u → %\"PRIu32\")\n2. ✅ 미사용 변수 제거 (s_first_frame_saved)\n3. ✅ 상세한 디버그 로그 추가\n4. ✅ errno 기반 에러 메시지 추가\n\n### 현재 세션 수정:\n5. ✅ I2C SCL 핀 설정 수정 (GPIO8 → GPIO6)\n\n===========================================\n작업 완료 - AI 어시스턴트 Claude\n===========================================\n"
  },
  "sections": {
    "문제 상황": "사용자가 제공한 실행 로그:",
    "원인 분석": "### 2.1 회로도 분석 (example/guide/회로도.pdf)",
    "해결 방법": "### 3.1 sdkconfig 파일 수정",
    "기대 효과": "I2C 핀이 올바르게 설정되면:",
    "하드웨어 확인 사항": "펌웨어 업로드 후 다음 사항을 확인하세요:",
    "예상 로그 출력": "수정 후 정상 작동 시 예상되는 로그:",
    "변경된 파일": "```",
    "참고 문서": "- 회로도: example/guide/회로도.pdf (Page 2)",
    "관련 칩 사양": "- **ESP32-P4**: 듀얼 코어 360MHz RISC-V, 32MB PSRAM",
    "다음 단계": "1. **펌웨어 플래시**: 수정된 설정으로 재빌드 및 업로드",
    "문제 해결 히스토리": "### 이전 세션에서 수정한 항목:"
  }
};
