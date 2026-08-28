# Fxco Device Binder (FXCO 스마트 행거 디바이스 바인더)

이 애플리케이션은 **FXCO(패션 익스피리언스 컴플렉스)** 프로젝트의 일환으로, 스마트 행거(Smart Hanger) 및 스마트 랙(Smart Rack) 디바이스를 의류(Garment) 및 시스템에 바인딩하고 관리하기 위한 Flutter 기반 모바일 앱입니다.

## 🚀 주요 역할
- **디바이스 바인딩**: QR 코드 스캔을 통해 스마트 행거를 특정 의류와 연결합니다.
- **실시간 상태 관리**: MQTT 프로토콜을 사용하여 디바이스의 연결 상태 및 이벤트를 실시간으로 모니터링하고 제어합니다.
- **NFC 태깅**: 스마트 랙의 NFC 정보를 읽어 디바이스 및 위치 정보를 확인합니다.
- **현장 관리**: 매장 내 스마트 행거와 랙의 동작 상태를 확인하고 필요 시 초기화하거나 설정을 변경합니다.

## 🛠 기술 스택
- **Framework**: Flutter (v3.5.0+)
- **State Management**: GetX
- **Communication**: MQTT (mqtt_client), HTTP (http)
- **Scanning & NFC**: mobile_scanner, nfc_manager
- **Permissions**: permission_handler

## 📂 프로젝트 구조 및 주요 기능 위치

```text
lib/
├── app/
│   ├── core/               # 공통 유틸리티 및 상수 (Snackbar, Constants 등)
│   ├── data/               # 데이터 레이어
│   │   ├── services/       # 데이터 통신 서비스
│   │   │   ├── api_service.dart   # API 서버 통신 (의류 정보 등 검색)
│   │   │   ├── mqtt_service.dart  # MQTT 브로커 연결 및 메시지 송수신 [핵심]
│   │   │   └── scan_service.dart  # QR/NFC 스캔 관련 비즈니스 로직
│   ├── modules/            # 기능별 모듈 (UI + Controller)
│   │   ├── home/           # 메인 대시보드 및 모듈 선택 화면
│   │   ├── smart_hanger/   # 스마트 행거 바인딩 및 제어 화면 [핵심]
│   │   │   ├── smart_hanger_page.dart        # QR 스캔 및 의류 매칭 UI
│   │   │   └── smart_hanger_controller.dart  # MQTT 구독 및 상태 처리 로직
│   │   └── smart_rack/     # 스마트 랙 NFC 정보 확인 화면
│   │       ├── smart_rack_page.dart          # NFC 정보 표시 UI
│   │       └── smart_rack_controller.dart    # NFC 데이터 처리 및 Canonical Hex 변환 로직
│   ├── theme/              # 앱 디자인 테마 및 스타일 (Colors, Typography)
│   └── widgets/            # 공통 재사용 위젯 (StatusChip, CustomButtons 등)
└── main.dart               # 앱 진입점 및 초기 설정
```

## 💡 주요 기능 설명

### 1. 스마트 행거 바인딩 (`smart_hanger`)
- **QR 스캔**: 스마트 행거에 부착된 QR 코드를 스캔하여 디바이스 ID를 획득합니다.
- **MQTT 연결**: 얻은 ID를 바탕으로 `smart_hanger/command/{id}` 및 `smart_hanger/event/{id}` 토픽을 구독합니다.
- **상세 제어**: 의류를 선택한 후 행거를 'Restart' 하거나 상태를 업데이트하는 명령을 전송합니다.

### 2. 스마트 랙 관리 (`smart_rack`)
- **NFC 리딩**: 스마트 랙에 스마트폰을 태깅하여 정보를 읽어옵니다.
- **데이터 변환**: 읽어온 NFC 원본 데이터를 `_toCanonicalHex` 포맷으로 변환하여 시스템 식별이 용이하게 표시합니다.

### 3. 실시간 상태 피드백
- MQTT연결 상태 및 작업 성공/실패 여부를 상단 **StatusChip** 및 **Snackbar**를 통해 사용자에게 즉각적으로 전달합니다.

## 🏁 시작하기

1. **의존성 설치**:
   ```bash
   flutter pub get
   ```
2. **권한 설정**: 카메라(QR 스캔) 및 NFC 권한이 필요합니다.
3. **실행**:
   ```bash
   flutter run
   ```

---
*개발 및 유지보수 시 `lib/app/data/services/mqtt_service.dart`의 브로커 정보를 확인하십시오.*
