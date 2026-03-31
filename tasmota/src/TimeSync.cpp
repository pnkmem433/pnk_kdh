#include "TimeSync.h"

#include <ESP8266WiFi.h>

TimeSync::TimeSync() : client_(udp_, "pool.ntp.org", 9 * 3600, 60000) {}

void TimeSync::begin() {
  client_.begin();
  tick();
}

void TimeSync::tick() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();
  if (synced_ && now - lastUpdateMs_ < 60000UL) {
    return;
  }

  if (client_.update()) {
    synced_ = true;
    lastUpdateMs_ = now;
  }
}

bool TimeSync::synced() const { return synced_; }
