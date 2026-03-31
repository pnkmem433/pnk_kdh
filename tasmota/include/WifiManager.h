#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

class WifiManager {
public:
  WifiManager(const char* ssid, const char* password);

  void begin();
  void tick(unsigned long retryDelayMs);
  bool connected() const;
  IPAddress localIp() const;

private:
  const char* ssid_;
  const char* password_;
  unsigned long lastAttemptMs_ = 0;
};
