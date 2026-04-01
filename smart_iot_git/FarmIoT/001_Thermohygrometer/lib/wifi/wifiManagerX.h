#ifndef WIFI_MANAGER_X_H
#define WIFI_MANAGER_X_H

#include <Arduino.h>
#include <wifi.h>

#include <vector>

#include <../struct/wifiCredential.h>

struct wifiManagerXParams {
  bool isNeedsInternet;
  int timeout;
  WifiCredential rootWiFi;
};

enum class WifiStatus { none, connecting, connected };

class WifiManagerX {
 private:
  std::vector<WifiCredential> credentials;

  WifiCredential permanentCredential;
  bool hasPermanentCredential = false;
  WifiStatus wifiStatus;

  String prefix = "";

 public:
  // 생성자
  WifiManagerX(wifiManagerXParams wifiManagerXParams);

  void set(std::vector<WifiCredential> wifiCredential);
  void remove(String ssid);
  void reset();

  std::vector<String> scanWifi();
  std::vector<String> getSavedCredentials();

  void connect();
  bool connect(WifiCredential WifiCredential);

  void disconnect();
  WifiStatus getWifiStatus();
  String connectssid();

  bool isNeedsInternet = false;
  int timeout = 3000;

  void setPrefix(String prefix);
  bool runCommand(String command, std::function<void(String)> callback);
};

#endif  // WIFI_MANAGER_X_H
