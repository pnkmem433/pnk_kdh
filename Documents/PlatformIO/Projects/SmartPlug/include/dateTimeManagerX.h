#ifndef TIME_H
#define TIME_H

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wifi.h>

#include "dateTime.h"

class DateTimeManagerX {
 public:
  DateTimeManagerX();
  void begin();
  DateTime now();

  bool isTimeLoaded();

 private:
  WiFiUDP ntpUDP;
  NTPClient timeClient;

  uint64_t epochTime;
  uint64_t lastUpdateTime;

  bool isLeapYear(int year);
  int daysInMonth(int month, int year);
};

#endif