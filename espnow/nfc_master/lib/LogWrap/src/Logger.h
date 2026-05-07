#pragma once

#include <Arduino.h>
#include <stdarg.h>

namespace Logger {

struct Argument {
  enum Type : uint8_t { Int, Str } type;
  int i;
  const char *s;

  Argument(int v) : type(Int), i(v), s(nullptr) {}
  Argument(const char *v) : type(Str), i(0), s(v) {}
};

struct Print {
  const char *value;                 // printf 포맷 문자열
  std::initializer_list<Argument> args; // 포맷에 대응하는 인자 목록
};

void print(const Print &print);

} // namespace Logger
