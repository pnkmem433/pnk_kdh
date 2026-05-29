# ESP32-P4 동영상 녹화 프로젝트

## 프로젝트 개요

ESP32-P4 기반 보드(WT99P4C5-S1 1V1)를 사용하여 MIPI_CSI 카메라로 10초 동영상을 촬영하고 SD카드에 저장하는 프로젝트

## 하드웨어 구성

### 보드
- **모델**: WT99P4C5-S1 1V1 (ESP32-P4)
- **MCU**: ESP32-P4 (Dual-core RISC-V)
- **RAM**: SPIRAM 지원

### 주변장치
- **카메라 (J10)**: MIPI_CSI 카메라
  - 센서: SC2336
  - 해상도: 1920x1080 @ 30fps
  - 포맷: YUV420
  - I2C (SCCB): SCL=GPIO8, SDA=GPIO7

- **SD카드 (J12)**: SDMMC 인터페이스
  - 모드: 4-line SDMMC
  - 핀 할당:
    - CLK: GPIO43
    - CMD: GPIO44
    - D0: GPIO39
    - D1: GPIO40
    - D2: GPIO41
    - D3: GPIO42

## 구현 목표

### 기능 요구사항
1. MIPI_CSI 카메라로 10초 동영상 녹화
2. SD카드에 동영상 파일 저장 (MJPEG AVI 포맷)

### 로그 출력 요구사항
1. "카메라 초기화"
2. "SD카드 초기화"
3. "동영상 녹화시작"
4. "동영상 녹화중 X%"
5. "동영상 녹화 후 저장중 X%"
6. "저장완료"

## 프로젝트 구조

```
251020_pnklabmem203_esp32p4_switchbox_PoC/
├── main/
│   ├── main.c                  # 메인 애플리케이션
│   ├── app_video.c/h           # 카메라 초기화 및 프레임 캡처
│   ├── sd_card.c/h             # SD카드 초기화 및 파일 작업
│   ├── mjpeg_recorder.c/h      # MJPEG AVI 파일 생성
│   ├── mjpeg_spool.c/h         # Ring buffer 및 SD 스트리밍
│   ├── CMakeLists.txt          # 빌드 설정
│   ├── idf_component.yml       # 의존성 설정
│   └── Kconfig.projbuild       # 프로젝트 설정
├── example/
│   ├── video_lcd_display/      # MIPI_CSI 카메라 예제
│   ├── sdmmc/                  # SD카드 예제
│   └── 참고프로젝트/            # MJPEG 동영상 녹화 참고 프로젝트
├── sdkconfig.defaults          # ESP32-P4 기본 설정
├── Ai/
│   ├── PROJECT_CONTEXT.md      # 이 파일
│   ├── request/
│   │   ├── request(1).txt      # 초기 프로젝트 분석 및 구현
│   │   ├── request(2).txt      # IntelliSense 오류 및 폴더 구조
│   │   ├── request(3).txt      # 빌드 오류 해결 (경로 공백)
│   │   └── request(4).txt      # 대화 컨텍스트 관리
│   └── settings.local.json     # 로컬 설정
└── README.md
```

## 기술 스택

### ESP-IDF 컴포넌트
- **esp_video**: MIPI_CSI 카메라 인터페이스 (V4L2)
- **esp_driver_jpeg**: 하드웨어 JPEG 인코더
- **esp_driver_sdmmc**: SD카드 드라이버
- **esp_vfs + fatfs**: FAT 파일시스템
- **esp_timer**: 마이크로초 타이머

### 동영상 처리
- **포맷**: MJPEG (Motion JPEG)
- **컨테이너**: AVI
- **인코딩**: 하드웨어 JPEG 인코더 사용
- **버퍼링**: Ring buffer (약 118MB, SPIRAM)

## 구현 상태

### 완료된 작업
- [x] 프로젝트 초기 분석 및 구현 계획
- [x] main/main.c 기본 로직 구현
- [x] 카메라 초기화 모듈 (app_video.c/h)
- [x] SD카드 초기화 모듈 (sd_card.c/h)
- [x] MJPEG 녹화 모듈 (mjpeg_recorder.c/h, mjpeg_spool.c/h)
- [x] 한글 로그 메시지 추가
- [x] 빌드 설정 (CMakeLists.txt, idf_component.yml)
- [x] Ai/request/ 폴더 구조 정리
- [x] 대화 컨텍스트 관리 (Ai/PROJECT_CONTEXT.md)

### 주요 이슈 및 해결
1. **IntelliSense 오류** (request(2).txt)
   - CONFIG_* 매크로 인식 오류
   - 해결: 정상 동작 확인 (빌드 시 자동 생성)

2. **빌드 오류 - 경로 공백 문제** (request(3).txt)
   - 경로 `D:/04. pretests-iot/` 공백 인식 오류
   - 해결 방법: 폴더명을 `04_pretests-iot`로 변경 권장
   - 현재 상태: **미해결** (사용자 조치 필요)

### 다음 단계
- [ ] 폴더 경로 공백 문제 해결 (`04. pretests-iot` → `04_pretests-iot`)
- [ ] 빌드 및 테스트
  ```bash
  idf.py set-target esp32p4
  idf.py build
  idf.py -p COMX flash monitor
  ```
- [ ] 실제 하드웨어 테스트
- [ ] 버그 수정 및 최적화

## 개발 가이드

### 빌드 방법
```bash
# 타겟 설정 (최초 1회)
idf.py set-target esp32p4

# 설정 확인/변경 (필요시)
idf.py menuconfig

# 빌드
idf.py build

# 플래시 및 모니터링
idf.py -p COMX flash monitor
```

### Git 워크플로우
모든 작업 완료 시:
```bash
git add .
git commit -m "{업데이트 내용 요약}"
git push origin main
```

### 대화 컨텍스트 복원 방법
VSCode 재시작 시 다음과 같이 시작:
```
Ai/PROJECT_CONTEXT.md와 최근 request 파일들을 참고해서 작업 계속
현재 작업: [하려는 작업 설명]
```

## 작업 방식 (매우 중요!)

### 대화 기록 필수 작성
**모든 질문/답변은 반드시 Ai/request/request(N).txt 파일에 기록해야 합니다.**

#### 파일 명명 규칙
- 질문마다 새 파일 생성
- 번호는 순차적으로 증가: request(1).txt, request(2).txt, request(3).txt ...
- 위치: Ai/request/ 폴더

#### 파일 형식
```ㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋ
```

#### 기록 내용
1. **질문**: 사용자가 요청한 내용 그대로 복사
2. **답변**:
   - 문제 분석
   - 해결 방법
   - 수행한 작업
   - 코드 변경 사항
   - 결과 확인

### 체크리스트 작업 흐름
1. **명령 수신**: 사용자로부터 작업 요청 받음
2. **TodoWrite 도구 사용**: 체크리스트 생성
   - 실제 작업 항목들
   - 대화 기록 작성
   - Git add / Git commit / Git push
3. **순차 실행**: 각 항목을 하나씩 실행
   - 상태: pending → in_progress → completed
4. **필수 마무리**: 아래 단계 반드시 수행

### 필수 마무리 단계 (모든 작업 완료 후)
```
체크리스트:
- [ ] 대화 내용을 Ai/request/request(N).txt에 기록
- [ ] 깃허브 반영
```

#### Git 커밋 메시지 형식
```
[작업 요약 제목]

- 주요 변경사항 1
- 주요 변경사항 2
- 주요 변경사항 3
- request(N).txt 대화 기록 추가
```

### 표준 작업 흐름 예시
```
1. 사용자: "기능 A를 구현해주세요"
   ↓
2. Ai: TodoWrite로 체크리스트 생성
   - [ ] 기능 A 설계
   - [ ] 기능 A 코드 작성
   - [ ] 기능 A 테스트
   - [ ] request(N).txt 기록
   - [ ] git add
   - [ ] git commit
   - [ ] git push
   ↓
3. Ai: 각 작업 순차 실행
   ↓
4. Ai: 대화 기록 작성 (request(N).txt)
   ↓
5. Ai: Git 반영
   ↓
6. 완료!
```

## 주요 설정

### 동영상 설정
- **해상도**: 1920x1080
- **프레임레이트**: 30fps
- **녹화 시간**: 10초 (약 300 프레임)
- **JPEG 품질**: 30 (1-100)
- **출력 파일**: /sdcard/VIDEO001.AVI
- **예상 파일 크기**: 15-30MB

### 메모리 사용
- Ring Buffer: ~118MB (40 프레임, SPIRAM)
- Batch Buffer: ~15.6MB (5 프레임)
- JPEG 인코딩 버퍼: ~7.2MB

## 참고 자료

### 프로젝트 소스
- example/video_lcd_display/ - MIPI_CSI 카메라 사용 예제
- example/sdmmc/ - SD카드 사용 예제
- example/참고프로젝트/ - MJPEG 동영상 녹화 구현 참고

### ESP-IDF 문서
- ESP32-P4 Technical Reference Manual
- ESP-IDF Programming Guide
- esp_video API Reference

## 알려진 제약사항

### ESP-IDF 경로 요구사항
- 프로젝트 경로에 **공백 금지**
- 프로젝트 경로에 **특수문자 금지**
- 프로젝트 경로에 **한글 금지**
- ASCII 문자만 사용 권장

### 하드웨어 제약
- SPIRAM 필수 (대용량 버퍼 사용)
- SD카드 속도: Class 10 이상 권장
- MIPI_CSI 카메라: SC2336 센서 지원

## 버전 정보

- **생성일**: 2025-10-20
- **마지막 업데이트**: 2025-10-20
- **프로젝트 상태**: 개발 중 (빌드 오류 해결 필요)

---

**Note**: 이 문서는 프로젝트의 전체 컨텍스트를 제공합니다.
새로운 대화 세션 시작 시 이 파일을 참고하여 작업을 계속할 수 있습니다.
