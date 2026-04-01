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

  String toString() {
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour,
            minute, second);
    return String(buffer);
  }

  // fromString 메서드 추가
  bool fromString(const String& str) {
    int y, m, d, h, mi, s;
    int result = sscanf(str.c_str(), "%04d-%02d-%02d %02d:%02d:%02d", &y, &m, &d, &h, &mi, &s);
    
    if (result == 6) {
      year = y;
      month = m;
      day = d;
      hour = h;
      minute = mi;
      second = s;
      return true;  // 변환 성공
    }
    return false;  // 변환 실패
  }

  // isEmpty: 모든 값이 0이면 true
  bool isEmpty() const {
    return year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0;
  }

  // isNotEmpty: isEmpty의 반대
  bool isNotEmpty() const {
    return !isEmpty();
  }
};

#endif
