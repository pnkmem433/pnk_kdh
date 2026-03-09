#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <WiFi.h>

struct WifiInfo {
  String ssid;
  String password;
};

class WifiManager {
public:
  WifiManager(WifiInfo wifiInfo);
  void begin();
  IPAddress dnsAddress();
  bool isconnected();

private:
  String ssid;
  String password;

};

#endif
