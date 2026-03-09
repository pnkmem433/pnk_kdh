#include "dateTimeManagerX.h"

DateTimeManagerX::DateTimeManagerX()
    : timeClient(ntpUDP, "pool.ntp.org", 9 * 3600, 60000), lastUpdateTime(0) {}

void DateTimeManagerX::begin() {
  timeClient.begin();
  uint64_t time = (esp_timer_get_time() / 1000ULL);
  if (timeClient.update()) {
    epochTime = timeClient.getEpochTime();
    lastUpdateTime = time;
    Serial.println("초기 시간 설정됨.");
  } else {
    Serial.println("초기 시간 설정 실패.");
  }

  xTaskCreate(
      [](void *parameter) {
        DateTimeManagerX *dtManager = (DateTimeManagerX *)parameter;

        while (true) {
          uint64_t time = (esp_timer_get_time() / 1000ULL);
          if (WiFi.status() == WL_CONNECTED &&
              (!dtManager->isTimeLoaded() ||
               time - dtManager->lastUpdateTime > 6 * 60 * 60 * 1000)) {
            if (dtManager->timeClient.update()) {
              Serial.println("시간 업데이트됨.");
              dtManager->lastUpdateTime = time;
              dtManager->epochTime = dtManager->timeClient.getEpochTime();
            }
          }

          vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
      },
      "TimeUpdateTask", 10000, this, 3, NULL);
}

DateTime DateTimeManagerX::now() {
  if (!isTimeLoaded())
    return {0, 0, 0, 0, 0, 0, 0};

  uint64_t currentEpochTime = epochTime;
  uint64_t currentLastUpdateTime = lastUpdateTime;

  uint64_t elapsedMillis =
      (esp_timer_get_time() / 1000ULL) - currentLastUpdateTime;

  uint64_t elapsedSeconds = elapsedMillis / 1000;
  int millisPart = elapsedMillis % 1000;  // ✅ 밀리초 부분 추가

  uint64_t currentEpoch = currentEpochTime + elapsedSeconds;

  int year = 1970;
  uint64_t seconds = currentEpoch;

  while (seconds >= (isLeapYear(year) ? 31622400 : 31536000)) {
    seconds -= (isLeapYear(year) ? 31622400 : 31536000);
    year++;
  }

  int month = 1;
  while (seconds >= daysInMonth(month, year) * 86400) {
    seconds -= daysInMonth(month, year) * 86400;
    month++;
  }

  int day = seconds / 86400 + 1;
  seconds %= 86400;

  int hour = seconds / 3600;
  seconds %= 3600;
  int minute = seconds / 60;
  int second = seconds % 60;

  return {year, month, day, hour, minute, second, millisPart};  // ✅ 수정됨
}


bool DateTimeManagerX::isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DateTimeManagerX::daysInMonth(int month, int year) {
  if (month == 2)
    return isLeapYear(year) ? 29 : 28;
  if (month == 4 || month == 6 || month == 9 || month == 11)
    return 30;
  return 31;
}

bool DateTimeManagerX::isTimeLoaded() { return epochTime != 0; }