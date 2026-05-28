#ifndef SMARTPLUG_WIFI_MANAGER_H
#define SMARTPLUG_WIFI_MANAGER_H

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <functional>

struct WifiInfo {
  String ssid;
  String password;
};

class WifiManager {
public:
  explicit WifiManager(WifiInfo wifiInfo);
  ~WifiManager();

  void begin(const std::function<void(void)>& tick = nullptr, bool forceConfigPortal = false,
             const std::function<void(void)>& portalStarting = nullptr);
  bool openConfigPortal(const std::function<void(void)>& tick = nullptr,
                        const std::function<void(void)>& portalStarting = nullptr);
  void resetSettings();
  void loop();
  IPAddress dnsAddress();
  bool isConnected();

private:
  struct PortalHandle;

  bool connectWithStoredOrFallback(const std::function<void(void)>& tick);
  bool connectWithFallbackCredentials(const std::function<void(void)>& tick);
  bool startConfigPortal(const std::function<void(void)>& tick, bool forceConfigPortal,
                         const std::function<void(void)>& portalStarting);
  String makeAccessPointName() const;

  String ssid;
  String password;
  unsigned long _lastRetryMs = 0;
  PortalHandle* _portal = nullptr;
};

#endif
