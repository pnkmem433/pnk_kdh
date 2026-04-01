#ifndef TIME_H
#define TIME_H

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>   // ✅ Wifi.h -> WiFi.h 권장

#include "../struct/dateTime.h"

class DateTimeManagerX {
 public:
  DateTimeManagerX();
  void begin();
  DateTime now();
  bool isTimeLoaded();

 private:
  void trySyncTimeOnce();   // ✅ 추가(시작 시 동기화 함수)
  // 내부 유틸리티
  DateTime epochToDateTime(uint64_t epoch);
  bool isEpochPlausible(uint64_t newEpoch, uint64_t nowMs);
  bool isYearPlausible(int year);
  int compileYear();

  WiFiUDP ntpUDP;
  NTPClient timeClient;

  uint64_t epochTime;       // ✅ 생성자에서 0 초기화
  uint64_t lastUpdateTime;  // ✅ 생성자에서 0 초기화

  bool isLeapYear(int year);
  int daysInMonth(int month, int year);
};

#endif
