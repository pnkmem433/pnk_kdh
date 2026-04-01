#include "dateTimeManagerX.h"

// 64bit tear 방지용 락
static portMUX_TYPE g_timeMux = portMUX_INITIALIZER_UNLOCKED;

DateTimeManagerX::DateTimeManagerX()
    : timeClient(ntpUDP, "pool.ntp.org", 9 * 3600, 60000),
      epochTime(0),
      lastUpdateTime(0) {}

void DateTimeManagerX::begin() {
  timeClient.begin();

  // 시작 시: WiFi 연결 전이면 update가 자주 실패하므로 "성공 시에만" 반영
  trySyncTimeOnce();

  xTaskCreate(
      [](void *parameter) {
        DateTimeManagerX *dtManager = (DateTimeManagerX *)parameter;

        while (true) {
          const uint64_t nowMs = (esp_timer_get_time() / 1000ULL);

          // 1시간마다(또는 아직 시간 없으면) 동기화 시도
          if (WiFi.status() == WL_CONNECTED) {
            uint64_t lastMs;
            portENTER_CRITICAL(&g_timeMux);
            lastMs = dtManager->lastUpdateTime;
            portEXIT_CRITICAL(&g_timeMux);

            const bool needSync = (!dtManager->isTimeLoaded()) ||
                                  (nowMs - lastMs > 1ULL * 60 * 60 * 1000);

            if (needSync) {
              if (dtManager->timeClient.update()) {
                const uint64_t newEpoch = (uint64_t)dtManager->timeClient.getEpochTime();

                // 1차: 2000~2100 범위, 2차: 연도 및 드리프트 기반 가드(2036류 차단)
                if (newEpoch >= 946684800ULL && newEpoch <= 4102444800ULL &&
                    dtManager->isEpochPlausible(newEpoch, nowMs)) {
                  portENTER_CRITICAL(&g_timeMux);
                  dtManager->epochTime = newEpoch;
                  dtManager->lastUpdateTime = nowMs;
                  portEXIT_CRITICAL(&g_timeMux);
                  Serial.println("시간 업데이트됨.");
                } else {
                  // 이상값이면 반영 금지
                  Serial.printf("NTP epoch 이상/비현실값 무시: %llu\n", (unsigned long long)newEpoch);
                }
              }
            }
          }

          vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
      },
      "TimeUpdateTask", 10000, this, 3, NULL);
}

void DateTimeManagerX::trySyncTimeOnce() {
  // WiFi가 아직 아니면 그냥 패스(쓰레기 epochTime 남지 않게 이미 0으로 초기화됨)
  if (WiFi.status() != WL_CONNECTED) return;

  const uint64_t nowMs = (esp_timer_get_time() / 1000ULL);

  // NTPClient는 상황에 따라 update()가 바로 실패할 수 있어 forceUpdate 한번 더 시도 가능
  bool ok = timeClient.update();
  if (!ok) ok = timeClient.forceUpdate();

  if (ok) {
    const uint64_t newEpoch = (uint64_t)timeClient.getEpochTime();
    if (newEpoch >= 946684800ULL && newEpoch <= 4102444800ULL &&
        isEpochPlausible(newEpoch, nowMs)) {
      portENTER_CRITICAL(&g_timeMux);
      epochTime = newEpoch;
      lastUpdateTime = nowMs;
      portEXIT_CRITICAL(&g_timeMux);
    } else {
      Serial.printf("NTP epoch 이상/비현실값 무시: %llu\n", (unsigned long long)newEpoch);
    }
  }
}

DateTime DateTimeManagerX::now() {
  uint64_t baseEpoch;
  uint64_t baseMs;

  // 스냅샷: tear 방지
  portENTER_CRITICAL(&g_timeMux);
  baseEpoch = epochTime;
  baseMs = lastUpdateTime;
  portEXIT_CRITICAL(&g_timeMux);

  // 로드 안됨
  if (baseEpoch == 0) return {0, 0, 0, 0, 0, 0};

  // 이상값 가드(혹시라도 메모리 오염/tear 등)
  if (baseEpoch < 946684800ULL || baseEpoch > 4102444800ULL) {
    return {0, 0, 0, 0, 0, 0};
  }

  const uint64_t nowMs = (esp_timer_get_time() / 1000ULL);
  const uint64_t elapsedSeconds = (nowMs - baseMs) / 1000ULL;
  const uint64_t currentEpoch = baseEpoch + elapsedSeconds;

  int year = 1970;
  uint64_t seconds = currentEpoch;

  while (seconds >= (isLeapYear(year) ? 31622400ULL : 31536000ULL)) {
    seconds -= (isLeapYear(year) ? 31622400ULL : 31536000ULL);
    year++;
  }

  int month = 1;
  while (seconds >= (uint64_t)daysInMonth(month, year) * 86400ULL) {
    seconds -= (uint64_t)daysInMonth(month, year) * 86400ULL;
    month++;
  }

  int day = (int)(seconds / 86400ULL) + 1;
  seconds %= 86400ULL;

  int hour = (int)(seconds / 3600ULL);
  seconds %= 3600ULL;
  int minute = (int)(seconds / 60ULL);
  int second = (int)(seconds % 60ULL);

  return {year, month, day, hour, minute, second};
}

bool DateTimeManagerX::isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DateTimeManagerX::daysInMonth(int month, int year) {
  if (month == 2) return isLeapYear(year) ? 29 : 28;
  if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
  return 31;
}

bool DateTimeManagerX::isTimeLoaded() {
  uint64_t e;
  portENTER_CRITICAL(&g_timeMux);
  e = epochTime;
  portEXIT_CRITICAL(&g_timeMux);
  return e != 0;
}

// ----- 내부 유틸리티 -----

// 컴파일 시각의 연도 (e.g. 2025)
int DateTimeManagerX::compileYear() {
  const char *d = __DATE__; // "Mmm dd yyyy"
  // 연도는 index 7~10
  int y = 0;
  for (int i = 7; d[i] && i < 11; ++i) {
    if (d[i] >= '0' && d[i] <= '9') y = y * 10 + (d[i] - '0');
  }
  if (y < 2000 || y > 2100) y = 2025; // 안전 기본값
  return y;
}

// 연도 가드: 2020 이상, 컴파일년도+5 이하
bool DateTimeManagerX::isYearPlausible(int year) {
  const int cy = compileYear();
  const int upper = cy + 5; // 장비 수명 고려 여유 5년
  if (year < 2020) return false;
  if (year > upper) return false;
  return true;
}

// epoch -> DateTime 변환(UTC offset 반영된 epoch 전제)
DateTime DateTimeManagerX::epochToDateTime(uint64_t epoch) {
  // epoch를 now()와 동일한 방식으로 풀기
  int year = 1970;
  uint64_t seconds = epoch;

  while (seconds >= (isLeapYear(year) ? 31622400ULL : 31536000ULL)) {
    seconds -= (isLeapYear(year) ? 31622400ULL : 31536000ULL);
    year++;
  }

  int month = 1;
  while (seconds >= (uint64_t)daysInMonth(month, year) * 86400ULL) {
    seconds -= (uint64_t)daysInMonth(month, year) * 86400ULL;
    month++;
  }

  int day = (int)(seconds / 86400ULL) + 1;
  seconds %= 86400ULL;

  int hour = (int)(seconds / 3600ULL);
  seconds %= 3600ULL;
  int minute = (int)(seconds / 60ULL);
  int second = (int)(seconds % 60ULL);

  return {year, month, day, hour, minute, second};
}

// NTP 값 타당성 체크: 기본 범위 + 연도 가드 + 기존 시간 대비 드리프트 가드
bool DateTimeManagerX::isEpochPlausible(uint64_t newEpoch, uint64_t nowMs) {
  // 1) 연도 가드(2036 류 방지)
  DateTime dt = epochToDateTime(newEpoch);
  if (!isYearPlausible(dt.year)) {
    return false;
  }

  // 2) 기존 시간과의 드리프트 가드(초기 로드면 생략)
  uint64_t baseEpoch;
  uint64_t baseMs;
  portENTER_CRITICAL(&g_timeMux);
  baseEpoch = epochTime;
  baseMs = lastUpdateTime;
  portEXIT_CRITICAL(&g_timeMux);

  if (baseEpoch != 0) {
    const uint64_t predicted = baseEpoch + ((nowMs - baseMs) / 1000ULL);
    int64_t diff = (int64_t)newEpoch - (int64_t)predicted;
    if (diff < 0) diff = -diff;
    // 6시간 이상 튀면 비정상으로 간주(오프셋/KST 고정이므로 DST 없음)
    if (diff > (int64_t)(6 * 60 * 60)) {
      return false;
    }
  }

  return true;
}
