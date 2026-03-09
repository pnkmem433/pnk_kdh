#ifndef DATETIME_STRUCT
#define DATETIME_STRUCT

#include <Arduino.h>

struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  int millisecond; // ✅ 추가

  String toString() {
    char buffer[24];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            year, month, day, hour, minute, second, millisecond);
    return String(buffer);
  }

  bool fromString(const String &str) {
    int y, m, d, h, mi, s, ms = 0;
    int result = sscanf(str.c_str(), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                        &y, &m, &d, &h, &mi, &s, &ms);

    if (result >= 6) { // 밀리초가 없더라도 성공
      year = y;
      month = m;
      day = d;
      hour = h;
      minute = mi;
      second = s;
      millisecond = (result == 7) ? ms : 0;
      return true;
    }
    return false;
  }

  bool isEmpty() const {
    return year == 0 && month == 0 && day == 0 &&
           hour == 0 && minute == 0 && second == 0 && millisecond == 0;
  }

  bool isNotEmpty() const { return !isEmpty(); }

  bool isLeapYear(int y) const {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
  }

  int daysInMonth(int y, int m) const {
    if (m == 2)
      return isLeapYear(y) ? 29 : 28;
    if (m == 4 || m == 6 || m == 9 || m == 11)
      return 30;
    return 31;
  }

  unsigned long long toUnixTime() const {
    unsigned long long days = 0;

    // 연도 기준 누적 일수
    for (int y = 1970; y < year; y++) {
      days += isLeapYear(y) ? 366 : 365;
    }

    // 월 기준 누적 일수
    for (int m = 1; m < month; m++) {
      days += daysInMonth(year, m);  // ✅ 인자 순서 수정됨
    }

    // 일 누적 (현재 일은 제외 → day - 1)
    days += (day - 1);

    // 총 초 계산
    unsigned long long totalSeconds =
        days * 86400ULL + hour * 3600ULL + minute * 60ULL + second;

    // 밀리초 포함
    return totalSeconds * 1000ULL + millisecond;
  }
};

#endif
