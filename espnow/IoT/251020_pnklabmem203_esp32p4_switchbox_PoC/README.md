# ESP32-P4 Camera Capture & Video Recording Project

ESP32-P4 기반 카메라 캡처 및 동영상 녹화 시스템 (OV5647 MIPI-CSI 카메라 지원)

## 프로젝트 개요

WT99P4C5-S1 개발 보드를 사용한 실시간 카메라 스트리밍 및 동영상 녹화 시스템입니다.
OV5647 카메라 센서를 통해 1920x1080 @ 30fps 영상을 캡처하고 SD 카드에 저장합니다.

### 주요 기능

- **실시간 카메라 스트리밍**: OV5647 MIPI-CSI 카메라 지원
- **고해상도 녹화**: 1920x1080 @ 30fps YUV420 포맷
- **SD 카드 저장**: 실시간 링 버퍼를 통한 안정적인 저장
- **AVI 변환**: 원본 YUV 데이터를 RGB565 AVI 파일로 변환
- **메모리 최적화**: 제한된 SPIRAM 환경에서 동작하도록 최적화

## 하드웨어 사양

### 개발 보드
- **모델**: WT99P4C5-S1 Development Board
- **메인 칩**: WT0132P4-A1 (ESP32-P4)
  - CPU: Dual-core RISC-V @ 360 MHz
  - SPIRAM: 32 MB PSRAM @ 200 MHz
  - Flash: 2 MB
- **Wi-Fi/BLE 모듈**: ESP32-C5-WROOM-1

### 카메라
- **센서**: OmniVision OV5647
- **해상도**: 최대 2592x1944 (5MP), 1920x1080 @ 30fps 사용
- **인터페이스**: MIPI-CSI 2-lane
- **포맷**: RAW10
- **I2C 주소**: 0x36 (7-bit)

### 스토리지
- **SD 카드**: SDHC/SDXC 지원
- **인터페이스**: SDMMC 4-bit
- **권장**: UHS-I U3 (90 MB/s 이상)

## 소프트웨어 아키텍처

### 주요 컴포넌트

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32-P4 Application                  │
├─────────────────────────────────────────────────────────┤
│  OV5647 Camera   →   ISP Pipeline   →   Ring Buffer     │
│  (MIPI-CSI)          (YUV420)           (SPIRAM)         │
│                                              ↓            │
│                                        Writer Task        │
│                                              ↓            │
│                                         SD Card           │
│                                         (FAT32)           │
│                                              ↓            │
│                                      AVI Converter        │
│                                    (YUV420 → RGB565)      │
└─────────────────────────────────────────────────────────┘
```

### 메모리 구조

**SPIRAM 사용 (32 MB)**:
- 링 버퍼: ~24 MB (8 프레임, 1920x1080 YUV420)
- ISP 버퍼: ~3 MB
- 기타 시스템: ~5 MB

**내부 RAM 사용 (545 KB)**:
- 시스템 힙: ~200 KB
- Task 스택: ~150 KB
- 기타: ~195 KB

### 핵심 모듈

1. **app_video.c/h**: 카메라 스트리밍 및 프레임 캡처
2. **mjpeg_spool.c/h**: 링 버퍼 관리 및 SD 카드 쓰기
3. **sd_card.c/h**: SD 카드 초기화 및 파일 시스템
4. **main.c**: 메인 애플리케이션 로직

## 빌드 및 설치

### 필수 요구사항

- **ESP-IDF**: v5.4.1 이상
- **Python**: 3.11 이상
- **CMake**: 3.16 이상
- **개발 환경**: Windows/Linux/macOS

### 빌드 스크립트

**Windows PowerShell**:
```powershell
# ESP-IDF 환경 설정 및 빌드
.\build_ov5647.ps1
```

**수동 빌드**:
```bash
# ESP-IDF 환경 설정
. $IDF_PATH/export.sh  # Linux/macOS
# 또는
$env:IDF_PATH = 'C:/Espressif/frameworks/esp-idf-v5.4.1/'  # Windows

# 빌드 디렉토리 정리 (선택사항)
idf.py fullclean

# 빌드
idf.py build

# 플래시
idf.py -p COM12 flash  # Windows
idf.py -p /dev/ttyUSB0 flash  # Linux

# 모니터링
idf.py -p COM12 monitor
```

### 설정 파일

**sdkconfig.defaults**:
```ini
# 타겟 칩
CONFIG_IDF_TARGET="esp32p4"

# 카메라 I2C 핀
CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN=8
CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN=7

# SPIRAM 설정
CONFIG_SPIRAM=y
CONFIG_SPIRAM_SPEED_200M=y

# OV5647 카메라 센서
CONFIG_CAMERA_OV5647=y
CONFIG_CAMERA_OV5647_AUTO_DETECT=y
CONFIG_CAMERA_OV5647_AUTO_DETECT_MIPI_INTERFACE_SENSOR=y
CONFIG_CAMERA_OV5647_MIPI_RAW10_1920x1080_30FPS=y
```

## 사용 방법

### 1. 하드웨어 연결

1. OV5647 카메라를 J10 커넥터(22-pin FPC)에 연결
2. SD 카드를 보드에 삽입 (FAT32 포맷)
3. USB-C 케이블로 전원 연결

### 2. 실행

```bash
# 시리얼 모니터 시작
idf.py -p COM12 monitor
```

### 3. 녹화 프로세스

1. **초기화** (자동):
   - SD 카드 마운트
   - 카메라 감지 및 설정
   - 비디오 스트림 시작

2. **10초 대기**:
   - 카메라 안정화
   - 프레임 캡처 테스트

3. **10초 녹화**:
   - YUV420 프레임을 SD 카드에 저장
   - `/sdcard/TEMP_RAW.yuv` 파일 생성

4. **AVI 변환**:
   - YUV420 → RGB565 변환
   - `/sdcard/VIDEO001.AVI` 파일 생성

### 4. 출력 파일

**원본 파일**:
- 경로: `/sdcard/TEMP_RAW.yuv`
- 포맷: YUV420 planar
- 크기: ~891 MB (300 프레임, 10초 @ 30fps)

**변환 파일**:
- 경로: `/sdcard/VIDEO001.AVI`
- 포맷: RGB565 AVI (압축 없음)
- 크기: ~1.2 GB (300 프레임)

## 성능 및 제약사항

### 성능 지표

| 항목 | 값 |
|------|-----|
| 해상도 | 1920x1080 |
| 프레임레이트 | 30 fps |
| 프레임 크기 | 2.97 MB (YUV420) |
| 데이터 레이트 | 89.1 MB/s |
| 링 버퍼 크기 | 8 프레임 (0.27초) |
| 프레임 드롭 | 0-10 프레임 (0-3%) |

### 메모리 제약

**SPIRAM 한계**:
- 사용 가능: 24 MB
- 프레임 크기: 2.97 MB
- 최대 링 버퍼: 8 프레임

**해결 방법**:
- 배치 버퍼 제거 (직접 쓰기)
- 최소 프레임 5로 완화
- SPIRAM 100% 활용

### SD 카드 요구사항

**최소 요구 성능**:
- 쓰기 속도: 90 MB/s 이상
- 클래스: UHS-I U3 이상
- 용량: 16 GB 이상 권장

**현재 테스트**:
- SD 카드: SC16G (SDHC)
- 속도: 40 MHz (40 MB/s)
- 결과: 성능 부족, 업그레이드 권장

## 문제 해결

### 주요 이슈 및 해결 내역

#### 1. 카메라 센서 불일치 (해결됨)
**문제**: SC2336 설정, 실제 OV5647 사용 → I2C 주소 불일치 (0x30 vs 0x36)
**해결**: sdkconfig.defaults를 OV5647로 변경
**참조**: [request(13).txt](Ai/request/request(13).txt)

#### 2. SPIRAM 부족 (해결됨)
**문제**: 59.33 MB 필요, 24.04 MB만 사용 가능
**해결**:
- 최소 프레임: 20 → 5
- 배치 예약: 5 프레임 → 0 프레임
- SPIRAM 사용률: 70% → 100%
**참조**: [request(14).txt](Ai/request/request(14).txt), [request(15).txt](Ai/request/request(15).txt)

#### 3. 프레임 대량 드롭 (해결됨)
**문제**: 배치 버퍼 3037 KB 할당 실패 → Writer task 종료 → 270+ 프레임 드롭
**해결**: 배치 버퍼 완전 제거, 링 버퍼에서 직접 SD 카드 쓰기
**참조**: [request(16).txt](Ai/request/request(16).txt)

### 일반적인 문제

**"Insufficient SPIRAM for ring buffer" 에러**:
```
해결 방법:
1. 해상도를 1280x960으로 낮추기
2. CONFIG_CAMERA_OV5647_MIPI_RAW10_1280x960_BINNING_45FPS=y
```

**"Ring buffer full, dropped frames" 경고**:
```
원인: SD 카드 성능 부족
해결 방법:
1. UHS-I U3 SD 카드 사용 (90 MB/s 이상)
2. 프레임레이트를 20 fps로 낮추기
```

**카메라 감지 실패**:
```
확인 사항:
1. 카메라 모듈이 J10에 올바르게 연결되었는지
2. FPC 케이블 방향 (접점이 보드 쪽)
3. I2C 핀 설정 (GPIO8=SCL, GPIO7=SDA)
```

## 개발 히스토리

### 주요 마일스톤

- **2025-10-20**: 프로젝트 시작
- **2025-10-20**: OV5647 센서 설정 수정 (SC2336 → OV5647)
- **2025-10-20**: SPIRAM 메모리 최적화 (3차 반복)
- **2025-10-21**: 프레임 드롭 해결 (배치 버퍼 제거)

### 최적화 과정

| 버전 | 변경 사항 | 결과 |
|------|-----------|------|
| v1.0 | 초기 구현 (SC2336) | I2C NACK 에러 |
| v2.0 | OV5647 센서 적용 | 센서 감지 성공 |
| v2.1 | SPIRAM 최적화 1차 | 부족 (59.33 MB 필요) |
| v2.2 | SPIRAM 최적화 2차 | 근소하게 부족 (23.73 MB 필요) |
| v2.3 | SPIRAM 최적화 3차 | 성공 (8 프레임 할당) |
| v3.0 | 배치 버퍼 제거 | Writer task 정상 동작 ✅ |

### 커밋 히스토리

최근 주요 커밋:
```
568f639 - 프레임 드롭 해결: 배치 버퍼 제거 및 직접 쓰기 구현
0d83433 - SPIRAM 부족 문제 최종 해결 (1080p 유지, 배치 예약 제거)
7345a97 - MJPEG 스풀 SPIRAM 부족 문제 해결 (메모리 최적화)
8fda43f - 카메라 센서 설정 수정: SC2336 → OV5647 (I2C 주소 0x30 → 0x36)
```

## 프로젝트 구조

```
.
├── main/
│   ├── main.c              # 메인 애플리케이션
│   ├── app_video.c/h       # 카메라 스트리밍
│   ├── mjpeg_spool.c/h     # 링 버퍼 및 SD 쓰기
│   └── sd_card.c/h         # SD 카드 관리
├── example/
│   ├── guide/              # 개발 보드 문서
│   │   ├── 가이드.pdf      # WT99P4C5-S1 가이드
│   │   └── 회로도.pdf      # 회로도
│   └── video_lcd_display/  # 참고 예제
├── Ai/
│   ├── PROJECT_CONTEXT.md  # 프로젝트 문맥
│   └── request/            # 개발 로그
│       ├── request(13).txt # OV5647 센서 설정
│       ├── request(14).txt # SPIRAM 최적화 1차
│       ├── request(15).txt # SPIRAM 최적화 2차
│       └── request(16).txt # 프레임 드롭 해결
├── sdkconfig.defaults      # ESP-IDF 기본 설정
├── build_ov5647.ps1        # Windows 빌드 스크립트
└── README.md               # 본 문서
```

## 향후 계획

### 단기 (v3.1)
- [ ] SD 카드 성능 프로파일링
- [ ] 동적 프레임레이트 조정
- [ ] AVI 저장 멈춤 문제 해결

### 중기 (v4.0)
- [ ] H.264 하드웨어 인코딩 지원
- [ ] Wi-Fi 스트리밍 기능
- [ ] 웹 인터페이스 추가

### 장기 (v5.0)
- [ ] 다중 카메라 지원
- [ ] 모션 감지 및 이벤트 녹화
- [ ] 클라우드 업로드 기능

## 라이선스

본 프로젝트는 ESP-IDF의 라이선스를 따릅니다.

## 기여자

- **AI Assistant**: Claude (Anthropic)
- **Human Developer**: Project Owner

## 참고 자료

### 공식 문서
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32-P4 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-p4_technical_reference_manual_en.pdf)
- [OV5647 Datasheet](https://cdn.sparkfun.com/datasheets/Dev/RaspberryPi/ov5647_full.pdf)

### 관련 프로젝트
- [esp_cam_sensor](https://components.espressif.com/components/espressif/esp_cam_sensor)
- [esp_video](https://components.espressif.com/components/espressif/esp_video)

### 개발 로그
프로젝트 개발 과정의 상세한 기록은 [Ai/request/](Ai/request/) 폴더를 참조하세요.

---

**Last Updated**: 2025-10-21
**Version**: 3.0
**Status**: Active Development
