#ifndef DATETIME_MANAGER_X_H
#define DATETIME_MANAGER_X_H

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "dateTime.h"

class DateTimeManagerX {
public:
  DateTimeManagerX();

  void begin();
  void update();
  DateTime now();
  bool isTimeLoaded();

private:
  bool isLeapYear(int year);
  int daysInMonth(int month, int year);

  WiFiUDP ntpUDP;
  NTPClient timeClient;
  unsigned long lastUpdateTime;
  unsigned long epochTime;
};

#endif
