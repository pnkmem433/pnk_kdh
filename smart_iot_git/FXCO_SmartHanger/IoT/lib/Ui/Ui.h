#pragma once

#include <Arduino.h>
#include <QRcode_eSPI.h>
#include <TFT_eSPI.h>
#include <clothData.h>

/*
  Ui 모듈
  - TFT 전체 화면을 한 프레임(sprite)로 그린 뒤 pushSprite()로 출력한다.
  - 주요 화면 모드:
    1) showAllColors(): 상품 정보 + 색상 + 가격
    2) showQr(): QR 코드 화면
    3) showNotify(): 서버 알림 강조 화면
    4) showQrMsg()/showText()/showLogo(): 안내/부팅 화면
  - 외부 상태 입력:
    - setCloth(), setNotify(), setCharge(), setPickdown(), setMqttConnected()
*/
class Ui {
public:
  Ui(TFT_eSPI *tft, TFT_eSprite *spr, TFT_eSprite *qspr, QRcode_eSPI *qrcode);

  // TFT + sprite 초기화
  void begin();

  // 전체 화면 alpha(0~1) 페이드 강도
  void setAlpha(float a);

  // 기본 배경색
  void setBaseBackground(uint16_t bgColor);

  // 행거 UUID (QR/디버그 표시 등에 사용)
  void setHangerUuid(String uuid);

  // 의류 데이터 모델 반영
  void setCloth(ClothData cloth);

  // 메인 화면 모드
  void showAllColors();
  void showQr();

  // 텍스트/안내/로고 화면
  void showText(const String &Title, const String &msg, uint16_t titleColor,
                uint16_t bodyColor);
  void showQrMsg(const String &title, const String &msg);
  void showLogo();
  void showLogo(float progress);

  // 알림 상태
  void setNotify(String name, uint32_t color);
  void removeNotify();
  void showNotify();

  // 네트워크 오버레이
  void setNetWork(String ssid, String ip, int32_t rssi, String uuid);
  void removeNetWork();

  // 충전/픽다운 상태
  void setCharge(bool isCharging);
  void setPickdown(bool state);

  // 링 애니메이션 타이밍 동기화 API
  void setRingAnimationAnchor(uint32_t anchorMs);
  void resetRingAnimation();

  // MQTT 연결 표시 배지 상태
  void setMqttConnected(bool connected);

private:
  bool mqttConnected = true;
  uint32_t mqttLastSeenMs = 0;

private:
  void drawMqttStatusBadge();

  // 색상/문자열 보조 함수
  uint16_t color24to16(uint32_t c);
  uint16_t blend565(uint16_t fg, uint16_t bg) const;
  String fmtKRW(int n);

  // 칩(rounded badge) 형태 텍스트
  void drawChipText(const String &text, int x, int y, uint16_t fgColor,
                    uint16_t bgColor);

  // 링 렌더링
  void fillArcBand(int cx, int cy, float startDeg, float sweepDeg, int rInner,
                   int rOuter, uint16_t color);
  void drawOuterRing();

private:
  // QR 알파 합성, 부가 아이콘/오버레이
  void drawQrAlpha(int dstX, int dstY, uint16_t fgColor565,
                   uint16_t bgColor565);
  void fillArcBandFine(int cx, int cy, float startDeg, float sweepDeg,
                       int rInner, int rOuter, uint16_t color);
  void drawLightningIcon(int x, int y, uint16_t color);
  void drawNetworkInfo(int centerX, int startY);

private:
  TFT_eSPI *tft = nullptr;
  TFT_eSprite *spr = nullptr;
  TFT_eSprite *qspr = nullptr;
  QRcode_eSPI *qrcode = nullptr;

  float alpha = 1.0f;
  uint16_t baseBg = TFT_BLACK;

  String uuid = "";

  ClothData cloth;

  bool isCharging = false;
  bool isPickdown = false;

  String notifyName = "";
  uint32_t notifyColor = 0xFFFFFF;

  // 마지막으로 생성한 QR 텍스트 캐시
  String lastQrText = "";

  // 링 애니메이션 기준 시각
  uint32_t ringAnimAnchorMs = 0;
  bool ringAnimAnchorValid = false;
  bool isPickdownPre = false;
  bool ringPrevValid = false;
  float ringPrevHeadStart = 0.0f;

  float lastAlpha = -1.0f;

  // 네트워크 오버레이 정보
  String ssid;
  String ip;
  int32_t rssi;
};
