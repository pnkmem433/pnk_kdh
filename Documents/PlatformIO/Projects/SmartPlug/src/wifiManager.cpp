#include "WifiManager.h"

WifiManager::WifiManager(WifiInfo wifiInfo) {
  ssid = wifiInfo.ssid;
  password = wifiInfo.password;
}

void WifiManager::begin() {
  // WiFi 연결 시도
  WiFi.disconnect(true, true);

  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("WiFi 연결 중...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi 연결 완료");

  // 로컬 IP 출력
  Serial.print("로컬 IP: ");
  Serial.println(WiFi.localIP());

  WiFi.setAutoReconnect(true);
}

IPAddress WifiManager::dnsAddress() { return WiFi.dnsIP(); }

bool WifiManager::isconnected() { return WiFi.status() == WL_CONNECTED; }