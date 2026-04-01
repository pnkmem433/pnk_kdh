/*
  [파일 설명] lib/clothData/ClothData.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "ClothData.h"
#include <ArduinoJson.h>

// parseHexColorToRGB565():
// "#RRGGBB" 또는 "RRGGBB" 문자열을 RGB565(16bit)로 변환
static uint16_t parseHexColorToRGB565(const char *s) {
  if (!s)
    return 0;
  if (s[0] == '#')
    s++;

  uint32_t rgb888 = (uint32_t)strtoul(s, nullptr, 16);

  uint8_t r = (rgb888 >> 16) & 0xFF;
  uint8_t g = (rgb888 >> 8) & 0xFF;
  uint8_t b = rgb888 & 0xFF;

  // RGB888 -> RGB565 변환
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

ClothData ClothData::fromString(const String &json) {
  // 출력 구조체 기본값 초기화
  ClothData out;
  out.price = 0;
  out.webSite = "";
  out.sizeColorOptions.clear();

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(err.f_str());
    return out;
  }

  // price
  out.price = doc["price"] | 0;

  // web_site
  out.webSite = doc["web_site"].as<String>();

  // size_color_options[] 파싱
  // 각 size 항목마다 colors 배열을 RGB565로 변환해 저장한다.
  JsonArray arr = doc["size_color_options"].as<JsonArray>();
  for (JsonObject item : arr) {
    SizeColorOption sco;
    sco.size = item["size"] | 0;
    sco.colors.clear();

    JsonArray colors = item["colors"].as<JsonArray>();
    for (JsonVariant v : colors) {
      const char *hex = v.as<const char *>();
      sco.colors.push_back(parseHexColorToRGB565(hex));
    }

    out.sizeColorOptions.push_back(sco);
  }

  return out;
}

