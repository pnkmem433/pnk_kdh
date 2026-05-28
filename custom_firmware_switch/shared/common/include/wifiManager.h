#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

struct WifiInfo {
  String ssid;
  String password;
};

class WifiManager {
public:
  explicit WifiManager(WifiInfo wifiInfo);

  void begin();
  void loop();
  IPAddress dnsAddress();
  bool isConnected();
  bool isconnected();

private:
  String ssid;
  String password;
};

#endif
