#pragma once

#include <Arduino.h>

namespace HexFormat {

struct ToString {
  uint32_t value;  // 변환할 값
  uint8_t width;   // 자리수(0이면 기본 2자리)
  bool upper;      // 대문자 사용 여부
  bool prefix;     // 0x/0X 접두사 포함 여부
};

String toString(const ToString &config);

} // namespace HexFormat
