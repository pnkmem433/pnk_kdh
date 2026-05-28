#include "Display.h"

#include <math.h>

Display::Display(const Config& config)
    : _config(config),
      _tft(),
      _sprite(&_tft),
      _spriteReady(false),
      _task(config.task),
      _loading(false),
      _frame(0),
      _lastFrameMs(0),
      _loadingToken(0),
      _loadingTitle(""),
      _loadingValue(""),
      _loadingIntervalMs(config.loadingIntervalMs == 0 ? 100 : config.loadingIntervalMs) {}

void Display::begin() {
  _tft.init();
  _tft.setRotation(_config.rotation);
  _tft.fillScreen(TFT_BLACK);
  delay(50);

  if (_config.powerPin >= 0) {
    pinMode(_config.powerPin, OUTPUT);
    digitalWrite(_config.powerPin, _config.powerHigh ? HIGH : LOW);
    if (_config.powerDelayMs > 0) {
      delay(_config.powerDelayMs);
    }
  }

  int16_t w = _tft.width();
  int16_t h = _tft.height();
  _spriteReady = _sprite.createSprite(w, h) != nullptr;
  if (_spriteReady) {
    _sprite.setTextDatum(TC_DATUM);
  }

  if (_task) {
    _task->begin({
        .loop = nullptr,
        .loopWithContext = Display::taskLoopThunk,
        .context = this,
    });
  }
}

void Display::showText(const ShowText& config) {
  portENTER_CRITICAL(&_mux);
  _loading = false;
  _loadingToken++;
  portEXIT_CRITICAL(&_mux);

  portENTER_CRITICAL(&_drawMux);
  TFT_eSprite* g = _spriteReady ? &_sprite : nullptr;
  if (g) {
    g->fillSprite(TFT_BLACK);
  } else {
    _tft.fillScreen(TFT_BLACK);
  }

  const char* title = config.title ? config.title : "";
  const char* value = config.value ? config.value : "";

  int16_t w = _tft.width();
  int16_t h = _tft.height();

  if (g) {
    g->setTextDatum(TC_DATUM);
    g->setTextColor(TFT_YELLOW);
  } else {
    _tft.setTextDatum(TC_DATUM);
    _tft.setTextColor(TFT_YELLOW);
  }

  if (g) {
    g->setTextSize(2);
  } else {
    _tft.setTextSize(2);
  }

  for (int i : {w / 2 - 1, w / 2, w / 2 + 1}) {
    if (g) {
      g->drawString(title, i, 8);
    } else {
      _tft.drawString(title, i, 8);
    }
  }

  if (g) {
    g->drawFastHLine(10, 26, w - 20, TFT_DARKGREY);
  } else {
    _tft.drawFastHLine(10, 26, w - 20, TFT_DARKGREY);
  }

  if (g) {
    g->setTextDatum(MC_DATUM);
    g->setTextColor(TFT_PURPLE);
  } else {
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_PURPLE);
  }

  const int16_t maxWidth = w - 20;
  const int16_t lineHeight = g ? g->fontHeight() : _tft.fontHeight();
  const int16_t lineGap = 2;
  const uint8_t maxLines = 4;

  String lines[maxLines];
  uint8_t lineCount = 0;

  String current;
  String token;
  for (const char* p = value;; ++p) {
    char c = *p;
    if (c == '\r') {
      continue;
    }
    if (c == '\n' || c == ':' || c == '\0') {
      if (token.length() > 0) {
        String candidate = current;
        if (candidate.length() > 0) {
          candidate += ":";
        }
        candidate += token;

        if ((g ? g->textWidth(candidate) : _tft.textWidth(candidate)) <= maxWidth) {
          current = candidate;
        } else {
          if (lineCount < maxLines) {
            if (current.length() > 0) {
              lines[lineCount++] = current;
              current = token;
            } else {
              lines[lineCount++] = token;
              current = "";
            }
          } else {
            current = token;
          }
        }
        token = "";
      }

      if (c == '\n') {
        if (lineCount < maxLines) {
          lines[lineCount++] = current;
        }
        current = "";
      }

      if (c == '\0') {
        break;
      }
    } else if (c != ' ') {
      token += c;
    }
  }

  if (current.length() > 0 && lineCount < maxLines) {
    lines[lineCount++] = current;
  }

  if (lineCount == 0) {
    lines[lineCount++] = "";
  }

  int16_t totalHeight = lineCount * lineHeight + (lineCount - 1) * lineGap;
  int16_t startY = (h - totalHeight) / 2 + 8;

  for (uint8_t i = 0; i < lineCount; i++) {
    int16_t y = startY + i * (lineHeight + lineGap);
    if (g) {
      g->drawString(lines[i], w / 2, y);
    } else {
      _tft.drawString(lines[i], w / 2, y);
    }
  }
  if (g) {
    g->pushSprite(0, 0);
  }
  portEXIT_CRITICAL(&_drawMux);
}

void Display::startLoading(const ShowText& config) {
  portENTER_CRITICAL(&_mux);
  _loadingTitle = config.title ? config.title : "";
  _loadingValue = config.value ? config.value : "";
  _loading = true;
  _frame = 0;
  _lastFrameMs = 0;
  _loadingToken++;
  portEXIT_CRITICAL(&_mux);

  showLoading({
      .title = _loadingTitle.c_str(),
      .value = _loadingValue.c_str(),
      .frame = _frame,
  });
}

void Display::stopLoading() {
  portENTER_CRITICAL(&_mux);
  _loading = false;
  _loadingToken++;
  portEXIT_CRITICAL(&_mux);
}

void Display::taskLoopThunk(void* context) { static_cast<Display*>(context)->taskLoop(); }

void Display::taskLoop() {
  bool loading = false;
  uint8_t frame = 0;
  String titleCopy;
  String valueCopy;
  uint32_t token = 0;

  portENTER_CRITICAL(&_mux);
  loading = _loading;
  if (loading) {
    uint32_t now = millis();
    if (_lastFrameMs == 0 || (now - _lastFrameMs) >= _loadingIntervalMs) {
      _lastFrameMs = now;
      _frame = (uint8_t)((_frame + 1) % 12);
      frame = _frame;
      titleCopy = _loadingTitle;
      valueCopy = _loadingValue;
      token = _loadingToken;
    } else {
      portEXIT_CRITICAL(&_mux);
      delay(20);
      return;
    }
  }
  portEXIT_CRITICAL(&_mux);

  if (!loading) {
    delay(20);
    return;
  }

  bool stillLoading = false;
  portENTER_CRITICAL(&_mux);
  stillLoading = _loading && (_loadingToken == token);
  portEXIT_CRITICAL(&_mux);
  if (!stillLoading) {
    return;
  }

  showLoading({
      .title = titleCopy.c_str(),
      .value = valueCopy.c_str(),
      .frame = frame,
  });
}

void Display::showLoading(const ShowLoading& config) {
  portENTER_CRITICAL(&_drawMux);
  TFT_eSprite* g = _spriteReady ? &_sprite : nullptr;
  if (g) {
    g->fillSprite(TFT_BLACK);
  } else {
    _tft.fillScreen(TFT_BLACK);
  }

  const char* title = config.title ? config.title : "";
  const char* value = config.value ? config.value : "";

  int16_t w = _tft.width();
  int16_t h = _tft.height();

  if (g) {
    g->setTextDatum(TC_DATUM);
    g->setTextColor(TFT_YELLOW);
    g->setTextSize(2);
  } else {
    _tft.setTextDatum(TC_DATUM);
    _tft.setTextColor(TFT_YELLOW);
    _tft.setTextSize(2);
  }
  for (int i : {w / 2 - 1, w / 2, w / 2 + 1}) {
    if (g) {
      g->drawString(title, i, 8);
    } else {
      _tft.drawString(title, i, 8);
    }
  }
  if (g) {
    g->drawFastHLine(10, 26, w - 20, TFT_DARKGREY);
  } else {
    _tft.drawFastHLine(10, 26, w - 20, TFT_DARKGREY);
  }

  int16_t cx = w / 2;
  int16_t cy = h / 2 + 6;
  int16_t r = 18;
  uint8_t frame = (uint8_t)(config.frame % 12);

  // Subtle ring
  if (g) {
    g->drawCircle(cx, cy, r + 3, TFT_DARKGREY);
    g->drawCircle(cx, cy, r + 4, TFT_DARKGREY);
  } else {
    _tft.drawCircle(cx, cy, r + 3, TFT_DARKGREY);
    _tft.drawCircle(cx, cy, r + 4, TFT_DARKGREY);
  }

  auto color565 = [](uint8_t r, uint8_t g, uint8_t b) -> uint16_t {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  };

  for (uint8_t i = 0; i < 12; i++) {
    float ang = (float)i * 30.0f * 3.1415926f / 180.0f;
    int16_t x = (int16_t)(cx + r * cosf(ang));
    int16_t y = (int16_t)(cy + r * sinf(ang));

    uint8_t dist = (uint8_t)((i + 12 - frame) % 12);
    uint8_t intensity = (uint8_t)(255 - dist * 18);  // trail fade
    if (intensity < 60) intensity = 60;

    uint16_t color = color565(255, intensity, 40);
    uint8_t dotSize = (dist == 0) ? 3 : (dist < 3 ? 2 : 1);
    if (g) {
      g->fillCircle(x, y, dotSize, color);
    } else {
      _tft.fillCircle(x, y, dotSize, color);
    }
  }

  const uint8_t maxLines = 3;
  String lines[maxLines];
  uint8_t lineCount = 0;
  String current;
  for (const char* p = value;; ++p) {
    char c = *p;
    if (c == '\r') {
      continue;
    }
    if (c == '\n' || c == '\0') {
      if (lineCount < maxLines) {
        lines[lineCount++] = current;
      }
      current = "";
      if (c == '\0') {
        break;
      }
    } else {
      current += c;
    }
  }
  if (lineCount == 0) {
    lines[lineCount++] = "";
  }

  int16_t lineHeight = g ? g->fontHeight() : _tft.fontHeight();
  int16_t lineGap = 2;
  int16_t totalHeight = lineCount * lineHeight + (lineCount - 1) * lineGap;
  int16_t startY = (h - 12) - totalHeight + lineHeight;

  if (g) {
    g->setTextDatum(MC_DATUM);
    g->setTextColor(TFT_ORANGE);
    g->setTextSize(1);
    for (uint8_t i = 0; i < lineCount; i++) {
      int16_t y = startY + i * (lineHeight + lineGap);
      g->drawString(lines[i], w / 2, y);
    }
    g->pushSprite(0, 0);
  } else {
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_ORANGE);
    _tft.setTextSize(1);
    for (uint8_t i = 0; i < lineCount; i++) {
      int16_t y = startY + i * (lineHeight + lineGap);
      _tft.drawString(lines[i], w / 2, y);
    }
  }
  portEXIT_CRITICAL(&_drawMux);
}
