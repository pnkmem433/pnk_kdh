#pragma once

#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeSync {
public:
  TimeSync();

  void begin();
  void tick();
  bool synced() const;

private:
  WiFiUDP udp_;
  NTPClient client_;
  unsigned long lastUpdateMs_ = 0;
  bool synced_ = false;
};
