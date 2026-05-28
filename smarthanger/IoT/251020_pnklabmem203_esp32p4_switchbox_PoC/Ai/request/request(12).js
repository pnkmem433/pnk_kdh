// request(12).js
window.requestData = window.requestData || [];
window.requestData[12] = {
  "request_number": 12,
  "title": "I2C 핀 설정 오류 수정: GPIO6 → GPIO8로 복원",
  "date": "2025-10-20",
  "author": "Claude (AI Assistant)",
  "status": "completed",
  "tags": [
    "request"
  ],
  "problem": {
    "summary": "### 1.1 실행 로그 분석"
  },
  "solution": {
    "summary": "### 2.1 회로도 오해"
  },
  "raw_content": {
    "question": "문제 재발생\n\n### 1.1 실행 로그 분석\n```\nI (1833) app_main: I2C 핀 설정: SCL=GPIO6, SDA=GPIO7\nE (1840) i2c.master: I2C transaction unexpected nack detected\nE (1866) sc2336: Get sensor ID failed\nE (1869) esp_video_init: failed to detect MIPI-CSI camera sensor with address=30\nE (1892) app_video: 카메라 장치 열기 실패: errno=2 (No such file or directory)\n```\n\n**증상**: GPIO6으로 수정했는데도 여전히 I2C NACK 에러 발생\n\n### 1.2 사용자 지시사항\n- \"핀맵을 보고 해결하라\"\n- \"/example/video_lcd_display에 예제가 있어\"\n\n",
    "answer": "핀맵 재분석\n\n### 2.1 회로도 오해\n\n**이전 분석 (잘못됨)**:\n- 회로도 2페이지(ESP32-C5 섹션)를 참조\n- CAM_IO0 = GPIO6, CAM_IO1 = GPIO7\n- 결론: GPIO6이 맞다고 판단 ✗\n\n**문제점**:\n- 회로도 2페이지는 **ESP32-C5** 모듈의 핀 배치\n- 카메라는 **ESP32-P4**에 연결되어 있음!\n- ESP32-C5와 ESP32-P4는 별도 칩임\n\n### 2.2 올바른 핀맵 (WT99P4C5-S1 Development Board Guide)\n\n**가이드 12페이지 - Table 3: J6 Pin Arrangement**\n\nESP32-P4 GPIO 핀 배치:\n```\nNo.  Name   Function\n14   IO6    GPIO6, SPI2_HOLD_PAD, LP_GPIO6, TOUCH_CHANNEL4\n16   IO7    GPIO7, SPI2_CS_PAD, LP_GPIO7, TOUCH_CHANNEL5\n18   IO8    GPIO8, UART0_RTS_PAD, SPI2_D_PAD, LP_GPIO8, TOUCH_CHANNEL6\n```\n\n**결론**: IO8 = GPIO8이 카메라 I2C SCL에 사용됨\n\n### 2.3 예제 프로젝트 확인\n\n**example/video_lcd_display/sdkconfig.defaults**:\n```\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=8  ← GPIO8 사용!\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN=7  ← GPIO7 사용!\n```\n\n**검증**: 공식 예제도 GPIO8을 사용하고 있음!\n\n\n\n근본 원인\n\n### 3.1 칩 혼동\n\n**개발 보드 구조**:\n```\nWT99P4C5-S1 Development Board\n├── WT0132P4-A1 (ESP32-P4)    ← 카메라 연결됨\n└── ESP32-C5-WROOM-1           ← Wi-Fi/BLE 모듈\n```\n\n**카메라 연결**:\n- MIPI-CSI 인터페이스 → ESP32-P4\n- I2C (SCCB) 제어 → ESP32-P4의 GPIO8 (SCL), GPIO7 (SDA)\n\n### 3.2 잘못된 판단 과정\n\n1. 회로도 2페이지의 CAM_IO0/CAM_IO1 확인\n2. \"CAM_IO0 = GPIO6\" 으로 해석\n3. GPIO6이 맞다고 결론 ✗\n\n**실제**:\n- CAM_IO0/CAM_IO1은 ESP32-C5의 신호 이름\n- ESP32-P4의 카메라 I2C는 GPIO8/GPIO7 사용\n- 회로도의 다른 페이지를 확인했어야 함\n\n\n\n해결 방법\n\n### 4.1 sdkconfig.defaults 수정\n\n**수정 전 (잘못된 설정)**:\n```\n# Camera I2C pins (from schematic: CAM_IO0=GPIO6 SCL, CAM_IO1=GPIO7 SDA)\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=6\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN=7\n```\n\n**수정 후 (올바른 설정)**:\n```\n# Camera I2C pins (ESP32-P4 GPIO, per WT99P4C5-S1 pinout: IO8=GPIO8, IO6/IO7 are also available)\n# Reference: example/video_lcd_display and Development Board Guide Table 3\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=8\nCONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN=7\n```\n\n### 4.2 검증 근거\n\n1. **공식 예제**: example/video_lcd_display가 GPIO8 사용\n2. **핀맵 테이블**: Development Board Guide Table 3 (J6 핀 배치)\n3. **블록 다이어그램**: Figure 3에서 WT0132P4-A1 → CSI MIPI Camera 연결 확인\n\n\n\nI2C NACK 에러의 실제 원인\n\nGPIO를 GPIO8로 복원해도 여전히 문제가 있다면:\n\n### 5.1 하드웨어 체크리스트\n\n**카메라 연결**:\n- [ ] MIPI-CSI 카메라가 J10 커넥터에 연결되어 있는가?\n- [ ] FPC 케이블 방향이 올바른가? (접점이 보드 쪽)\n- [ ] 22핀 FPC 케이블이 사용되고 있는가?\n\n**전원**:\n- [ ] 카메라 전원(VCC_3V3) 공급 확인\n- [ ] 로그에서 LDO 초기화 성공 여부 확인\n\n**카메라 센서**:\n- [ ] SC2336 센서가 맞는가?\n- [ ] I2C 주소 0x30 (7-bit) 사용\n- [ ] 센서가 물리적으로 손상되지 않았는가?\n\n### 5.2 소프트웨어 체크리스트\n\n**설정 확인**:\n```bash\n# sdkconfig에서 카메라 설정 확인\ngrep CAMERA sdkconfig\n```\n\n예상 출력:\n```\nCONFIG_CAMERA_SC2336=y\nCONFIG_CAMERA_SC2336_MIPI_RAW8_1920x1080_30FPS=y\n```\n\n**Pull-up 저항**:\n- I2C SCL/SDA에 pull-up 저항 필요 (보드에 내장되어 있어야 함)\n- 로직 레벨: 3.3V\n\n\n\n예상 결과\n\n### 6.1 GPIO8 설정 후 빌드\n\n```bash\n# 클린 빌드\nidf.py fullclean\nidf.py build\nidf.py flash monitor\n```\n\n### 6.2 예상 로그 (카메라 연결 시)\n\n**정상 동작**:\n```\nI (xxxx) app_main: I2C 핀 설정: SCL=GPIO8, SDA=GPIO7\nI (xxxx) esp_video_init: Camera sensor detected: SC2336\nI (xxxx) app_main: 카메라 드라이버 초기화 완료\nI (xxxx) app_video: 카메라 장치 열기 성공 (fd=3)\nI (xxxx) TAG: 첫 프레임 캡쳐 성공!\n```\n\n**카메라 미연결**:\n```\nI (xxxx) app_main: I2C 핀 설정: SCL=GPIO8, SDA=GPIO7\nE (xxxx) i2c.master: I2C transaction unexpected nack detected\nE (xxxx) sc2336: Get sensor ID failed\n```\n→ 이 경우 하드웨어 연결 확인 필요\n\n\n\n학습 사항\n\n### 7.1 개발 보드 구조 이해의 중요성\n\n**WT99P4C5-S1 구성**:\n- 메인 프로세서: ESP32-P4 (WT0132P4-A1)\n- 무선 통신: ESP32-C5 (ESP32-C5-WROOM-1)\n- 카메라: ESP32-P4에 직접 연결\n\n### 7.2 문서 참조 방법\n\n**올바른 순서**:\n1. **개발 보드 가이드** 먼저 확인 (핀맵 테이블)\n2. **공식 예제** 참조 (example/)\n3. **회로도** 상세 분석 (칩별로 구분하여 확인)\n\n**잘못된 방법**:\n1. 회로도만 보고 판단\n2. 칩을 구분하지 않고 핀 이름만 확인\n3. 예제 코드 무시\n\n### 7.3 핀 번호 vs GPIO 번호\n\n**혼동 주의**:\n- **핀 번호**: 물리적 커넥터의 핀 위치 (1, 2, 3...)\n- **GPIO 번호**: 소프트웨어에서 사용하는 GPIO 번호\n- **IO 이름**: 개발 보드의 실크스크린 표시 (IO8 = GPIO8)\n\n\n\n다음 단계\n\n### 8.1 빌드 및 테스트\n\n1. **클린 빌드**:\n```bash\nidf.py fullclean\nidf.py build\n```\n\n2. **펌웨어 플래시**:\n```bash\nidf.py flash monitor\n```\n\n3. **로그 확인**:\n- I2C 핀 설정: SCL=GPIO8\n- 카메라 센서 감지 여부\n- 프레임 캡쳐 성공 여부\n\n### 8.2 하드웨어 확인 (에러 지속 시)\n\n**카메라 미연결 시**:\n- MIPI-CSI 카메라 모듈 연결\n- FPC 케이블 방향 확인\n- J10 커넥터 접촉 확인\n\n**센서 불량 시**:\n- 다른 카메라 모듈로 교체\n- I2C 버스 스캔으로 주소 확인\n\n\n\n파일 변경 사항\n\n```\n수정됨: sdkconfig.defaults\n  - CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN: 6 → 8\n  - 주석 업데이트: ESP32-P4 GPIO 명시\n  - 참조 문서 추가: example/video_lcd_display, Guide Table 3\n```\n\n\n\n참고 문서\n\n### 10.1 확인한 문서\n\n1. **WT99P4C5-S1 Development Board Guide v1.2**\n   - Page 6: Component Introduction\n   - Page 10-11: Block Diagram\n   - Page 12-13: Table 3 (J6 Pin Arrangement)\n\n2. **example/video_lcd_display**\n   - sdkconfig.defaults: GPIO8 사용 확인\n   - main/main.c: 카메라 초기화 코드\n\n3. **회로도.pdf**\n   - ESP32-C5와 ESP32-P4 구분 필요\n   - MIPI-CSI 연결 확인\n\n### 10.2 핵심 정보\n\n**ESP32-P4 카메라 I2C 핀**:\n- SCL: GPIO8 (J6 핀 18번, IO8)\n- SDA: GPIO7 (J6 핀 16번, IO7)\n\n**카메라 센서**:\n- 모델: SC2336\n- 해상도: 1920x1080@30fps\n- 포맷: RAW8\n- I2C 주소: 0x30\n\n\n\n결론\n\n**판단 오류**:\n- 회로도의 ESP32-C5 섹션을 ESP32-P4로 오해\n- GPIO6이 맞다고 잘못 결론\n\n**올바른 설정**:\n- GPIO8 (SCL) + GPIO7 (SDA)\n- 공식 예제 및 핀맵 테이블과 일치\n\n**남은 문제**:\n- GPIO8로 설정해도 I2C NACK 발생 시\n- → 하드웨어 연결 확인 필요 (카메라 미연결 가능성)\n\n===========================================\n작업 완료 - AI 어시스턴트 Claude\n===========================================\n"
  },
  "sections": {
    "문제 재발생": "### 1.1 실행 로그 분석",
    "핀맵 재분석": "### 2.1 회로도 오해",
    "근본 원인": "### 3.1 칩 혼동",
    "해결 방법": "### 4.1 sdkconfig.defaults 수정",
    "I2C NACK 에러의 실제 원인": "GPIO를 GPIO8로 복원해도 여전히 문제가 있다면:",
    "예상 결과": "### 6.1 GPIO8 설정 후 빌드",
    "학습 사항": "### 7.1 개발 보드 구조 이해의 중요성",
    "다음 단계": "### 8.1 빌드 및 테스트",
    "파일 변경 사항": "```",
    "참고 문서": "### 10.1 확인한 문서",
    "결론": "**판단 오류**:"
  }
};
