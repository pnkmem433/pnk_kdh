/*
  [파일 설명] lib/Ui/Ui.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
// ====================== Ui.cpp ======================
#include "Ui.h"
#include "picture.h"
#include <math.h>
#include <string.h>

/*
  Ui.cpp 안내
  - Smart Hanger의 TFT 화면 렌더링 로직을 구현한다.
  - 보통 spr(메인 sprite)에 먼저 그린 뒤 pushSprite()로 출력한다.
  - QR은 qspr(1bit) 버퍼를 사용해 필요할 때만 다시 생성한다.
*/

// ====================== 기본 ======================
Ui::Ui(TFT_eSPI *tft, TFT_eSprite *spr, TFT_eSprite *qspr, QRcode_eSPI *qrcode)
    : tft(tft), spr(spr), qspr(qspr), qrcode(qrcode) {}

// TFT, 메인 sprite, QR sprite를 초기화한다.
void Ui::begin() {
  tft->init();
  tft->setRotation(1);
  tft->fillScreen(baseBg);

  spr->setColorDepth(16);
  spr->createSprite(240, 240);
  spr->setTextFont(2);
  spr->setTextDatum(MC_DATUM);

  // QR Sprite는 1bit 버퍼로 사용
  qspr->setColorDepth(1);
  qspr->createSprite(100, 100);
  qspr->fillSprite(TFT_BLACK); // QR 버퍼 초기값

  qrcode->init();
}

// ====================== 설정 ======================
// alpha를 0.0~1.0 범위로 제한한다.
void Ui::setAlpha(float a) {
  if (a < 0.0f)
    a = 0.0f;
  if (a > 1.0f)
    a = 1.0f;
  alpha = a;
}

void Ui::setBaseBackground(uint16_t bgColor) { baseBg = bgColor; }
void Ui::setHangerUuid(String uuid) { this->uuid = uuid; }

// ====================== 유틸 ======================
// 24bit(0xRRGGBB)를 RGB565로 변환한다.
uint16_t Ui::color24to16(uint32_t c) {
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >> 8) & 0xFF;
  uint8_t b = c & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// alpha 비율을 이용해 전경/배경 색을 블렌딩한다.
uint16_t Ui::blend565(uint16_t fg, uint16_t bg) const {
  if (alpha <= 0.0f)
    return bg;
  if (alpha >= 1.0f)
    return fg;

  uint8_t fr = (fg >> 11) & 0x1F;
  uint8_t fg_g = (fg >> 5) & 0x3F;
  uint8_t fb = fg & 0x1F;

  uint8_t br = (bg >> 11) & 0x1F;
  uint8_t bg_g = (bg >> 5) & 0x3F;
  uint8_t bb = bg & 0x1F;

  uint8_t r = (uint8_t)(fr * alpha + br * (1.0f - alpha));
  uint8_t g = (uint8_t)(fg_g * alpha + bg_g * (1.0f - alpha));
  uint8_t b = (uint8_t)(fb * alpha + bb * (1.0f - alpha));

  return (r << 11) | (g << 5) | b;
}

// 숫자를 천 단위 콤마 문자열로 변환한다.
String Ui::fmtKRW(int n) {
  String s = String(n), out = "";
  int len = s.length(), cnt = 0;
  for (int i = len - 1; i >= 0; --i) {
    out = s[i] + out;
    cnt++;
    if (cnt == 3 && i != 0) {
      out = "," + out;
      cnt = 0;
    }
  }
  return out;
}

// ====================== 요소 그리기 ======================
// 라운드 칩 형태 배지를 그리고 텍스트를 표시한다.
void Ui::drawChipText(const String &text, int x, int y, uint16_t fgColor,
                      uint16_t bgColor) {
  if (alpha <= 0.0f)
    return;

  int textWidth = spr->textWidth(text) + 16;
  int textHeight = spr->fontHeight() + 4;
  int rx = x - textWidth / 2;
  int ry = y - textHeight / 2;
  int rr = min(min(textHeight, textWidth) / 2, 8);

  uint16_t bgBlend = blend565(bgColor, baseBg);
  uint16_t fgBlend = blend565(fgColor, baseBg);

  spr->fillRoundRect(rx, ry, textWidth, textHeight, rr, bgBlend);
  uint16_t borderBlend = blend565(TFT_WHITE, baseBg);
  spr->drawRoundRect(rx, ry, textWidth, textHeight, rr, borderBlend);

  spr->setTextDatum(MC_DATUM);
  spr->setTextColor(fgBlend, bgBlend);
  spr->drawString(text, x, y);
}

// ====================== 화면 ======================
void Ui::setCloth(ClothData cloth) { this->cloth = cloth; }

/*
  showAllColors()
  ------------------------------------------------------------------------------
  [역할]
  기본 상품 화면을 렌더링한다.

  [표시 항목]
  1) 상단 로고
  2) 사이즈 값
  3) 색상 원형 칩 목록
  4) 가격(KRW 포맷)
  5) 충전 아이콘 + 외곽 링 애니메이션
  6) (옵션) 네트워크 오버레이
  7) MQTT 상태 배지

  [주의]
  - cloth.sizeColorOptions가 비어있는 상황을 방어한다.
  - 마지막 pushSprite() 이전까지 실제 화면에는 반영되지 않는다.
*/
void Ui::showAllColors() {
  lastAlpha = alpha;
  spr->fillSprite(baseBg);
  spr->fillCircle(120, 120, 118, baseBg);

  // 로고
  spr->setSwapBytes(true);
  spr->pushImage(120 - inst.width / 2, 38 - inst.height / 2, inst.width,
                 inst.height, inst.data);

  // 사이즈
  spr->setTextColor(blend565(TFT_WHITE, baseBg));
  spr->setTextDatum(MC_DATUM);
  spr->setTextSize(3);

  int sizeVal = 0;
  if (!cloth.sizeColorOptions.empty())
    sizeVal = cloth.sizeColorOptions[0].size;

  for (int i = 119; i <= 121; i++) {
    spr->drawString(String(sizeVal), i, 90);
  }

  // 컬러 칩 (개수 제한 없음, 중앙 정렬)
  if (!cloth.sizeColorOptions.empty()) {
    int cy = 150;
    const int r = 12;
    const int spacing = 28; // 원이 겹치지 않게 간격 설정
    int n = (int)cloth.sizeColorOptions[0].colors.size();

    if (n > 0) {
      int totalWidth = (n - 1) * spacing;
      int startX = 120 - totalWidth / 2;

      for (int i = 0; i < n; i++) {
        int x = startX + i * spacing;
        uint16_t c = cloth.sizeColorOptions[0].colors[i]; // RGB565 그대로 사용
        spr->fillCircle(x, cy, r, blend565(c, baseBg));
        spr->drawCircle(x, cy, r, blend565(TFT_WHITE, baseBg));
      }
    }
  }

  // 가격
  spr->setTextColor(blend565(TFT_WHITE, baseBg));
  spr->setTextDatum(MC_DATUM);
  spr->setTextSize(1);
  spr->drawString("KRW " + fmtKRW(cloth.price), 120, 200);

  if (!isCharging)
    drawLightningIcon(120, 230, baseBg);
  drawOuterRing();
  if (isCharging)
    drawLightningIcon(120, 230, 0xFDE0);

  if (ssid != "") {
    drawNetworkInfo(120, 120);
  }

  drawMqttStatusBadge();

  spr->pushSprite(0, 0);
}

// ====================== QR 출력 ======================
/*
  showQr()
  ------------------------------------------------------------------------------
  [역할]
  QR 중심 화면을 렌더링한다.

  [QR 데이터 선택 규칙]
  - cloth.webSite가 있으면 webSite 사용
  - 없으면 uuid 사용

  [성능 최적화]
  - QR 텍스트가 바뀌지 않으면 qrcode->create()를 다시 호출하지 않는다.
  - qspr 캐시를 재사용해 프레임 비용을 줄인다.
*/
void Ui::showQr() {
  lastAlpha = alpha;
  spr->fillSprite(baseBg);
  spr->fillCircle(120, 120, 118, baseBg);

  // 로고
  spr->setSwapBytes(true);
  spr->pushImage(120 - inst.width / 2, 38 - inst.height / 2, inst.width,
                 inst.height, inst.data);

  int qx = (240 - 100) / 2;
  int qy = (240 - 100) / 2;

  // QR은 변경될 때만 생성(캐시)
  String qrText = cloth.webSite.length() ? cloth.webSite : uuid;
  if (qrText != lastQrText) {
    lastQrText = qrText;
    qspr->fillSprite(TFT_BLACK);
    qrcode->create(qrText.c_str());
  }

  // 알파 블렌딩 적용 렌더(spr에만 그림)
  drawQrAlpha(qx, qy, TFT_DARKGREY, baseBg);

  // SCAN ME
  uint16_t qrColor = blend565(TFT_WHITE, baseBg);
  spr->setTextColor(qrColor);
  spr->setTextDatum(MC_DATUM);
  spr->setTextSize(1);
  spr->drawString("SCAN ME!", 120, 200);

  if (!isCharging)
    drawLightningIcon(120, 230, baseBg);
  drawOuterRing();
  if (isCharging)
    drawLightningIcon(120, 230, 0xFDE0);

  if (ssid != "") {
    drawNetworkInfo(120, 120);
  }

  drawMqttStatusBadge();

  spr->pushSprite(0, 0);
}

/*
  showText()
  ------------------------------------------------------------------------------
  [역할]
  제목 + 본문 안내 화면을 표시한다.

  [줄바꿈]
  - 공백 단위로 단어를 누적해 폭을 검사한다.
  - 최대 폭을 넘으면 다음 줄로 넘긴다.
  - '\n' 문자를 만나면 즉시 줄을 종료한다.
*/
void Ui::showText(const String &Title, const String &msg, uint16_t titleColor,
                  uint16_t bodyColor) {
  spr->fillSprite(baseBg);
  spr->fillCircle(120, 120, 118, baseBg);

  spr->setTextDatum(TC_DATUM);
  spr->setTextColor(titleColor, baseBg);
  spr->setTextSize(2);
  for (int i = 119; i <= 121; i++)
    spr->drawString(Title, i, 34);

  spr->setTextDatum(TL_DATUM);
  spr->setTextColor(bodyColor, baseBg);
  spr->setTextSize(1);

  const int left = 30;
  const int right = 210;
  const int topY = 68;
  const int lineH = spr->fontHeight() + 2;

  auto wrapAndDraw = [&](const String &text, int x, int y, int maxW) {
    String line = "";
    int cursorY = y;

    String word = "";
    for (int i = 0; i <= text.length(); ++i) {
      char c = (i < text.length()) ? text[i] : '\n';
      if (c == ' ' || c == '\n') {
        String candidate = (line.length() ? line + " " + word : word);
        if (spr->textWidth(candidate) > maxW) {
          spr->drawString(line, x, cursorY);
          cursorY += lineH;
          line = word;
        } else {
          line = candidate;
        }
        word = "";
        if (c == '\n') {
          spr->drawString(line, x, cursorY);
          cursorY += lineH;
          line = "";
        }
      } else {
        word += c;
      }
    }
    if (line.length()) {
      spr->drawString(line, x, cursorY);
      cursorY += lineH;
    }
    return cursorY;
  };

  (void)wrapAndDraw(msg, left, topY, right - left);
  spr->pushSprite(0, 0);
}

/*
  showQrMsg()
  ------------------------------------------------------------------------------
  [역할]
  설정 안내 메시지 + QR 안내를 표시한다.

  [차이점]
  - showText()와 유사한 줄바꿈 로직을 사용하되 본문 출력 후 QR 블록을 추가한다.
  - QR 데이터는 uuid 고정값을 사용한다.
*/
void Ui::showQrMsg(const String &title, const String &msg) {
  spr->fillSprite(baseBg);
  spr->fillCircle(120, 120, 118, baseBg);

  spr->setTextDatum(TC_DATUM);
  spr->setTextColor(TFT_RED);
  spr->setTextSize(2);
  for (int i = 119; i <= 121; i++)
    spr->drawString(title, i, 30);

  spr->setTextDatum(TL_DATUM);
  spr->setTextColor(TFT_WHITE, baseBg);
  spr->setTextSize(1);

  const int left = 30;
  const int right = 210;
  const int topY = 64;
  const int lineH = spr->fontHeight() + 2;

  auto wrapAndDraw = [&](const String &text, int x, int y, int maxW) {
    String line = "", word = "";
    int cursorY = y;

    for (int i = 0; i <= text.length(); ++i) {
      char c = (i < text.length()) ? text[i] : '\n';
      if (c == ' ' || c == '\n') {
        String candidate = (line.length() ? line + " " + word : word);
        if (spr->textWidth(candidate) > maxW) {
          spr->drawString(line, x, cursorY);
          cursorY += lineH;
          line = word;
        } else {
          line = candidate;
        }
        word = "";
        if (c == '\n') {
          spr->drawString(line, x, cursorY);
          cursorY += lineH;
          line = "";
        }
      } else {
        word += c;
      }
    }

    if (line.length()) {
      spr->drawString(line, x, cursorY);
      cursorY += lineH;
    }
    return cursorY;
  };

  int afterBodyY = wrapAndDraw(msg, left, topY, right - left);

  if (!isCharging)
    drawLightningIcon(120, 230, baseBg);

  // QR은 uuid 고정
  String qrText = uuid;
  if (qrText != lastQrText) {
    lastQrText = qrText;
    qspr->fillSprite(TFT_BLACK);
    qrcode->create(qrText.c_str());
  }

  // 본문 아래에 QR 배치(가로 중앙)
  int qrX = (240 - 100) / 2;
  int qrY = afterBodyY + 10;
  drawQrAlpha(qrX, qrY, TFT_DARKGREY, baseBg);

  if (isCharging)
    drawLightningIcon(120, 230, 0xFDE0);

  spr->pushSprite(0, 0);
}

// 로고만 표시하는 대기 화면.
void Ui::showLogo() {
  spr->fillSprite(baseBg);
  spr->fillCircle(120, 120, 118, baseBg);

  int logoX = 120 - inst.width / 2;
  int logoY = 120 - inst.height / 2;
  spr->setSwapBytes(true);
  spr->pushImage(logoX, logoY, inst.width, inst.height, inst.data);

  spr->pushSprite(0, 0);
}

// OTA 진행 표시가 필요한 로고 화면.
void Ui::showLogo(float progress) {
  progress = constrain(progress, 0.0f, 1.0f);

  spr->fillSprite(baseBg);
  spr->fillCircle(120, 120, 118, baseBg);

  int logoX = 120 - inst.width / 2;
  int logoY = 120 - inst.height / 2;
  spr->setSwapBytes(true);
  spr->pushImage(logoX, logoY, inst.width, inst.height, inst.data);

  const int barW = 120;
  const int barH = 16;
  const int barR = 6;
  const int barX = 120 - barW / 2;
  const int barY = 190;

  spr->fillRoundRect(barX, barY, barW, barH, barR, TFT_WHITE);

  const int fillW = (int)(barW * progress + 0.5f);
  if (fillW > 0)
    spr->fillRoundRect(barX, barY, fillW, barH, barR, TFT_GREEN);

  spr->pushSprite(0, 0);
}

// notify 데이터 설정
void Ui::setNotify(String name, uint32_t color) {
  notifyName = name;
  notifyColor = color;
}

// notify 상태 초기화
void Ui::removeNotify() {
  notifyName = "";
  notifyColor = 0xFFFFFF;
}

// 알림 강조 화면 렌더링
void Ui::showNotify() {
  auto brightness = [](uint16_t color) -> int {
    int r = ((color >> 11) & 0x1F) << 3;
    int g = ((color >> 5) & 0x3F) << 2;
    int b = (color & 0x1F) << 3;
    return (r * 299 + g * 587 + b * 114) / 1000;
  };

  uint16_t textColor;
  if (brightness(color24to16(notifyColor)) > 128)
    textColor = TFT_BLACK;
  else
    textColor = TFT_WHITE;

  uint16_t notifyBlend = blend565(textColor, baseBg);
  uint16_t colorBlend = blend565(color24to16(notifyColor), baseBg);
  uint16_t whiteBlend = blend565(TFT_WHITE, baseBg);

  spr->fillSprite(baseBg);
  spr->fillCircle(120, 120, 118, colorBlend);

  spr->setTextColor(notifyBlend);
  spr->setTextSize(2);
  for (int dx = -1; dx <= 1; dx++)
    spr->drawString("Your choice", 120 + dx, 120 - (spr->fontHeight() / 2 + 5));

  drawChipText(notifyName, 120, 120 + (spr->fontHeight() / 2 + 5), TFT_BLACK,
               whiteBlend);

  spr->pushSprite(0, 0);
}

void Ui::setCharge(bool isCharging) { this->isCharging = isCharging; }
void Ui::setPickdown(bool state) { this->isPickdown = state; }

// ====================== 링 Arc 기반 그리기 ======================
// 지정 각도 범위를 링 밴드로 채워 그린다.
void Ui::fillArcBand(int cx, int cy, float startDeg, float sweepDeg, int rInner,
                     int rOuter, uint16_t color) {
  if (sweepDeg <= 0.0f)
    return;

  // draw 용 빠른 step
  const float stepDeg = 6.0f;

  while (startDeg < 0)
    startDeg += 360.0f;
  while (startDeg >= 360.0f)
    startDeg -= 360.0f;

  float endDeg = startDeg + sweepDeg;

  auto drawRange = [&](float a0, float a1) {
    for (float a = a0; a < a1; a += stepDeg) {
      float a2 = a + stepDeg;
      if (a2 > a1)
        a2 = a1;

      float r1 = a * 0.01745329252f;
      float r2 = a2 * 0.01745329252f;

      int x1 = cx + (int)(cosf(r1) * rInner);
      int y1 = cy + (int)(sinf(r1) * rInner);
      int x2 = cx + (int)(cosf(r1) * rOuter);
      int y2 = cy + (int)(sinf(r1) * rOuter);

      int x3 = cx + (int)(cosf(r2) * rOuter);
      int y3 = cy + (int)(sinf(r2) * rOuter);
      int x4 = cx + (int)(cosf(r2) * rInner);
      int y4 = cy + (int)(sinf(r2) * rInner);

      spr->fillTriangle(x1, y1, x2, y2, x3, y3, color);
      spr->fillTriangle(x1, y1, x3, y3, x4, y4, color);
    }
  };

  if (endDeg <= 360.0f) {
    drawRange(startDeg, endDeg);
  } else {
    drawRange(startDeg, 360.0f);
    drawRange(0.0f, endDeg - 360.0f);
  }
}

// ====================== 링 지우기(초세밀) ======================
// fillArcBand보다 촘촘하게 그려 지우기 품질을 높인다.
void Ui::fillArcBandFine(int cx, int cy, float startDeg, float sweepDeg,
                         int rInner, int rOuter, uint16_t color) {
  if (sweepDeg <= 0.0f)
    return;

  // clear 시 더 촘촘한 step(잔상 제거)
  const float stepDeg = 2.0f;

  while (startDeg < 0)
    startDeg += 360.0f;
  while (startDeg >= 360.0f)
    startDeg -= 360.0f;

  float endDeg = startDeg + sweepDeg;

  auto drawRange = [&](float a0, float a1) {
    for (float a = a0; a < a1; a += stepDeg) {
      float a2 = a + stepDeg;
      if (a2 > a1)
        a2 = a1;
      if (a2 <= a)
        continue;

      float r1 = a * 0.01745329252f;
      float r2 = a2 * 0.01745329252f;

      int x1 = cx + (int)(cosf(r1) * rInner);
      int y1 = cy + (int)(sinf(r1) * rInner);
      int x2 = cx + (int)(cosf(r1) * rOuter);
      int y2 = cy + (int)(sinf(r1) * rOuter);

      int x3 = cx + (int)(cosf(r2) * rOuter);
      int y3 = cy + (int)(sinf(r2) * rOuter);
      int x4 = cx + (int)(cosf(r2) * rInner);
      int y4 = cy + (int)(sinf(r2) * rInner);

      spr->fillTriangle(x1, y1, x2, y2, x3, y3, color);
      spr->fillTriangle(x1, y1, x3, y3, x4, y4, color);
    }
  };

  if (endDeg <= 360.0f) {
    drawRange(startDeg, endDeg);
  } else {
    drawRange(startDeg, 360.0f);
    drawRange(0.0f, endDeg - 360.0f);
  }
}

// ====================== 링 ======================
/*
  drawOuterRing()
  ------------------------------------------------------------------------------
  [역할]
  외곽 링의 tail/head 애니메이션을 렌더링한다.

  [pickdown 상태 처리]
  - isPickdown==true면 링을 비우고 애니메이션을 멈춘다.
  - 상태 전이 첫 프레임에서만 넓은 영역 clear를 수행한다.

  [일반 상태 처리]
  1) 이전 프레임 잔상 영역 부분 지우기
  2) 배경 링(360도) 다시 그리기
  3) tail segment 여러 개 + head segment를 순서대로 그리기
*/
void Ui::drawOuterRing() {
  // int count = 6;
  // for (int i = 0; i < count; i++) {

  //   // i=0 ??125, i=count-1 ??0
  //   uint8_t brightness = 125 - (i * (125 / (count - 1)));

  //   uint16_t color = tft->color565(brightness, brightness, brightness);
  //   spr->drawCircle(120, 120, 119 - i, !isPickdown ? color : TFT_BLACK);
  // }

  const int cx = 120, cy = 120;
  const int thickness = 6;
  const int rOuter = 119;
  const int rInner = rOuter - thickness;

  // pickdown 진입 시 1회 clear + 애니 상태 리셋
  if (isPickdown) {
    if (!isPickdownPre) {
      // 링 영역만 확실하게 지우기
      fillArcBandFine(cx, cy, 0.0f, 360.0f, rInner - 3, rOuter + 3, baseBg);
      ringPrevValid = false; // 다음 복귀 시 잔상 방지
    }
    isPickdownPre = true;
    return;
  }
  isPickdownPre = false;

  // ======================
  // 1) 이전 프레임 꼬리+헤드 범위만 지우기(잔상 제거 + FPS)
  // ======================
  // 시간(외부 동기화 anchor 지원)
  uint32_t now = millis();
  uint32_t baseMs = ringAnimAnchorValid ? ringAnimAnchorMs : now;
  float t = (now - baseMs) / 1000.0f;

  const float speedDegPerSec = 240.0f;
  const float headLen = 80.0f;
  const float tailLen = 160.0f;
  const int tailSteps = 8;

  float headStart = fmodf(t * speedDegPerSec, 360.0f);

  if (ringPrevValid) {
    // 이전에 그렸던 범위를 여유 있게 지운다
    const float margin = 12.0f;
    float clearStart = ringPrevHeadStart - (tailLen + headLen + margin);
    float clearSweep = (tailLen + headLen + margin * 2.0f);

    fillArcBandFine(cx, cy, clearStart, clearSweep, rInner - 2, rOuter + 2,
                    baseBg);
  }

  ringPrevHeadStart = headStart;
  ringPrevValid = true;

  // ======================
  // 2) 배경 링(360) 다시 그리기
  //    - 잔상 제거 목적이 아니라 기본 상태를 다시 표시하기 위한 처리
  // ======================
  uint16_t baseRing = tft->color565(40, 40, 40);
  fillArcBand(cx, cy, 0.0f, 360.0f, rInner, rOuter, baseRing);

  // ======================
  // 3) 꼬리 + 헤드 그리기
  // ======================
  for (int i = 0; i < tailSteps; i++) {
    float u = (float)i / (float)(tailSteps - 1);
    float fade = 1.0f - (u * u);

    int v = (int)(35 + 150 * fade);
    uint16_t c = tft->color565(v, v, v);

    float segLen = tailLen / tailSteps;
    float segStart = headStart - (i + 1) * segLen;
    fillArcBand(cx, cy, segStart, segLen, rInner, rOuter, c);
  }

  uint16_t head = tft->color565(235, 235, 235);
  fillArcBand(cx, cy, headStart, headLen, rInner, rOuter, head);
}

// ====================== 링 동기화 API ======================
void Ui::setRingAnimationAnchor(uint32_t anchorMs) {
  ringAnimAnchorMs = anchorMs;
  ringAnimAnchorValid = true;
}
void Ui::resetRingAnimation() {
  ringAnimAnchorValid = false;
  ringPrevValid = false;
}

// ====================== QR Alpha ======================
/*
  drawQrAlpha()
  ------------------------------------------------------------------------------
  [역할]
  qspr(1bit QR 버퍼)에서 검은색이 아닌 픽셀만 읽어
  메인 spr에 전경색으로 찍는다.

  [이렇게 하는 이유]
  - QR 버퍼를 1bit로 유지해 메모리를 절약
  - 메인 화면 alpha(blend565)와 자연스럽게 결합 가능
*/
void Ui::drawQrAlpha(int dstX, int dstY, uint16_t fgColor565,
                     uint16_t bgColor565) {
  if (alpha <= 0.0f)
    return;

  const int W = 100;
  const int H = 100;

  // qspr(1bit) 픽셀을 읽어 spr에 찍기
  uint16_t c = blend565(fgColor565, bgColor565);

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      if (qspr->readPixel(x, y) != TFT_BLACK) {
        spr->drawPixel(dstX + x, dstY + y, c);
      }
    }
  }
}

// 번개 아이콘(충전 상태 표시) 렌더링
void Ui::drawLightningIcon(int cx, int cy, uint16_t color) {
  int ax = cx - 1, ay = cy - 8;
  int bx = cx + 6, by = cy - 8;
  int cx1 = cx + 1, cy1 = cy - 1;
  int dx = cx + 7, dy = cy - 1;
  int ex = cx - 3, ey = cy + 8;
  int fx = cx + 0, fy = cy + 1;
  int gx = cx - 6, gy = cy + 1;
  spr->fillTriangle(ax, ay, bx, by, cx1, cy1, color);
  spr->fillTriangle(ax, ay, cx1, cy1, dx, dy, color);
  spr->fillTriangle(ax, ay, dx, dy, ex, ey, color);
  spr->fillTriangle(ax, ay, ex, ey, fx, fy, color);
  spr->fillTriangle(ax, ay, fx, fy, gx, gy, color);
  spr->drawPixel(cx + 7, cy - 1, color);
  spr->drawPixel(cx - 3, cy + 8, color);
}

// 네트워크 오버레이 데이터를 저장한다.
void Ui::setNetWork(String ssid, String ip, int32_t rssi, String uuid) {
  this->ssid = ssid;
  this->ip = ip;
  this->rssi = rssi;
  this->uuid = uuid;
}
// 네트워크 오버레이 데이터를 초기화한다.
void Ui::removeNetWork() {
  ssid = "";
  ip = "";
  rssi = 0;
  uuid = "";
}

// SSID/RSSI/IP/UUID 정보를 중앙 정렬로 출력한다.
void Ui::drawNetworkInfo(int centerX, int centerY) {
  if (ssid == "" && ip == "" && uuid == "")
    return;
  spr->setTextDatum(TC_DATUM); // 위 가운대 정렬
  spr->setTextSize(2);
  spr->setTextColor(TFT_WHITE, baseBg);

  const int maxW = 200; // 240 기준 좌우 여백 20
  const int lineH = spr->fontHeight();

  auto wrapAndDraw = [&](const String &text, int y) {
    String line = "";
    String word = "";
    int cursorY = y;

    for (int i = 0; i <= text.length(); ++i) {
      char c = (i < text.length()) ? text[i] : '\n';

      if (c == ' ' || c == '\n') {
        String candidate = line.length() ? line + " " + word : word;

        if (spr->textWidth(candidate) > maxW) {
          spr->drawString(line, centerX, cursorY);
          cursorY += lineH;
          line = word;
        } else {
          line = candidate;
        }

        word = "";

        if (c == '\n') {
          spr->drawString(line, centerX, cursorY);
          cursorY += lineH;
          line = "";
        }
      } else {
        word += c;
      }
    }

    if (line.length()) {
      spr->drawString(line, centerX, cursorY);
      cursorY += lineH;
    }

    return cursorY;
  };

  int y = centerY - (lineH * 3) / 2; // 3줄 기준 중앙 맞춤
  y = wrapAndDraw(ssid + "\n(" + rssi + "dBm)\n[" + ip +
                      "]\nuuid: " + uuid.substring(0, 6),
                  y);
}

void Ui::setMqttConnected(bool connected) { mqttConnected = connected; }

// MQTT가 끊긴 경우 하단에 작은 점 배지를 표시한다.
void Ui::drawMqttStatusBadge() {
  if (mqttConnected)
    return;

  // 은은한 회색
  uint16_t dot = blend565(tft->color565(120, 120, 120), baseBg);

  const int x = 120; // 하단 중앙
  const int y = 232; // 가장 아래쪽(240x240 기준)
  const int r = 3;   // 아주 작은 점

  spr->fillCircle(x, y, r, dot);
}

