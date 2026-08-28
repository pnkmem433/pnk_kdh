#include "HexAsciiConverter.h"

bool HexAsciiConverter::isAsciiPrintable(uint8_t val) {
  return (val >= 0x20 && val <= 0x7E);
}

String HexAsciiConverter::convert(const String &hexStr) {
  if (hexStr.length() <= 9) {
    return "";
  }
  String cleanHex = hexStr.substring(5, hexStr.length() - 4);
  String result = "";

  for (int i = 0; i < cleanHex.length(); i += 2) {
    String byteStr = cleanHex.substring(i, i + 2);
    uint8_t value = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    if (isAsciiPrintable(value)) {
      result += (char)value;
    }
  }

  return result;
}
