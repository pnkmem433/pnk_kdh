#include "dateTimeManagerX.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

DateTimeManagerX::DateTimeManagerX()
    : timeClient(ntpUDP, "pool.ntp.org", 9 * 3600, 60000),
      lastUpdateTime(0),
      epochTime(0) {}

void DateTimeManagerX::begin() {
  timeClient.begin();
  update();
}

void DateTimeManagerX::update() {
  const unsigned long nowMs = millis();
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!isTimeLoaded() || nowMs - lastUpdateTime > 21600000UL) {
    if (timeClient.update()) {
      epochTime = timeClient.getEpochTime();
      lastUpdateTime = nowMs;
      Serial.println("Time synced");
    }
  }
}

DateTime DateTimeManagerX::now() {
  if (!isTimeLoaded()) {
    return {0, 0, 0, 0, 0, 0, 0};
  }

  unsigned long currentEpoch = epochTime + ((millis() - lastUpdateTime) / 1000UL);
  unsigned long seconds = currentEpoch;
  int year = 1970;

  while (seconds >= (isLeapYear(year) ? 31622400UL : 31536000UL)) {
    seconds -= (isLeapYear(year) ? 31622400UL : 31536000UL);
    year++;
  }

  int month = 1;
  while (seconds >= (unsigned long)daysInMonth(month, year) * 86400UL) {
    seconds -= (unsigned long)daysInMonth(month, year) * 86400UL;
    month++;
  }

  const int day = (seconds / 86400UL) + 1;
  seconds %= 86400UL;

  const int hour = seconds / 3600UL;
  seconds %= 3600UL;
  const int minute = seconds / 60UL;
  const int second = seconds % 60UL;

  return {year, month, day, hour, minute, second, (int)((millis() - lastUpdateTime) % 1000UL)};
}

bool DateTimeManagerX::isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DateTimeManagerX::daysInMonth(int month, int year) {
  if (month == 2) {
    return isLeapYear(year) ? 29 : 28;
  }
  if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;
  }
  return 31;
}

bool DateTimeManagerX::isTimeLoaded() { return epochTime != 0; }
