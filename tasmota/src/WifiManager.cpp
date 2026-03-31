#include "WifiManager.h"

WifiManager::WifiManager(const char* ssid, const char* password)
    : ssid_(ssid), password_(password) {}

void WifiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid_, password_);
  lastAttemptMs_ = millis();
}

void WifiManager::tick(unsigned long retryDelayMs) {
  if (connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastAttemptMs_ < retryDelayMs) {
    return;
  }

  lastAttemptMs_ = now;
  WiFi.disconnect();
  WiFi.begin(ssid_, password_);
}

bool WifiManager::connected() const { return WiFi.status() == WL_CONNECTED; }

IPAddress WifiManager::localIp() const { return WiFi.localIP(); }
