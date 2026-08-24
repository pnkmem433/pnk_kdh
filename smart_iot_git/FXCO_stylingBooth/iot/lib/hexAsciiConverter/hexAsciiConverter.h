#ifndef HEX_ASCII_CONVERTER_H
#define HEX_ASCII_CONVERTER_H

#include <Arduino.h>

class HexAsciiConverter {
public:
    // ASCII 범위 판별
    static bool isAsciiPrintable(uint8_t val);

    // 앞 4바이트, 뒤 4바이트 제외하고 ASCII 변환
    static String convert(const String &hexStr);
};

#endif // HEX_ASCII_CONVERTER_H
