#include "SmartPlugWifiManager.h"

#include <WiFiManager.h>

namespace {
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000UL;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 30000UL;
constexpr unsigned long WIFI_PORTAL_TIMEOUT_SECONDS = 0UL;
}  // namespace

struct WifiManager::PortalHandle {
  ::WiFiManager wm;
};

WifiManager::WifiManager(WifiInfo wifiInfo) {
  ssid = wifiInfo.ssid;
  password = wifiInfo.password;
  _portal = new PortalHandle();
}

WifiManager::~WifiManager() {
  delete _portal;
  _portal = nullptr;
}

void WifiManager::begin(const std::function<void(void)>& tick, bool forceConfigPortal,
                        const std::function<void(void)>& portalStarting) {
#if defined(ESP8266)
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
#else
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
#endif

  if (!forceConfigPortal && connectWithStoredOrFallback(tick)) {
    return;
  }

  startConfigPortal(tick, forceConfigPortal, portalStarting);
}

bool WifiManager::openConfigPortal(const std::function<void(void)>& tick,
                                   const std::function<void(void)>& portalStarting) {
  return startConfigPortal(tick, true, portalStarting);
}

void WifiManager::resetSettings() {
  if (_portal == nullptr) {
    return;
  }
  _portal->wm.resetSettings();
}

bool WifiManager::connectWithStoredOrFallback(const std::function<void(void)>& tick) {
  if (WiFi.SSID().length() > 0) {
    Serial.print("Connecting to saved WiFi: ");
    Serial.println(WiFi.SSID());
    WiFi.begin();

    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      if (tick) {
        tick();
      }
      delay(100);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
  }

  if (connectWithFallbackCredentials(tick)) {
    return true;
  }

  Serial.println("WiFi fallback connection timeout");
  return false;
}

bool WifiManager::connectWithFallbackCredentials(const std::function<void(void)>& tick) {
  if (ssid.isEmpty()) {
    return false;
  }

  // Do not overwrite a user-saved Wi-Fi profile when trying the AppConfig fallback.
  WiFi.persistent(false);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("WiFi connecting");
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    if (tick) {
      tick();
    }
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  WiFi.persistent(true);

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool WifiManager::startConfigPortal(const std::function<void(void)>& tick, bool forceConfigPortal,
                                    const std::function<void(void)>& portalStarting) {
  if (_portal == nullptr) {
    return false;
  }

  auto& wm = _portal->wm;

#if defined(ESP8266)
  WiFi.mode(WIFI_AP_STA);
#endif

  wm.setDebugOutput(true);
  wm.setConfigPortalBlocking(false);
  wm.setConnectTimeout(WIFI_CONNECT_TIMEOUT_MS / 1000U);
  wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_SECONDS);
  wm.setBreakAfterConfig(true);

  const String apName = makeAccessPointName();
  Serial.println(forceConfigPortal ? "Starting forced WiFi config portal" : "Starting WiFi config portal");
  Serial.print("Config AP: ");
  Serial.println(apName);
  if (portalStarting) {
    portalStarting();
  }

  const bool portalStarted = forceConfigPortal
                                 ? wm.startConfigPortal(apName.c_str())
                                 : wm.autoConnect(apName.c_str());

  while (wm.getConfigPortalActive()) {
    wm.process();
    if (tick) {
      tick();
    }
    delay(10);
  }

  if (!portalStarted || WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi config portal ended without connection");
    return false;
  }

  Serial.println("WiFi connected from config portal");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void WifiManager::loop() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - _lastRetryMs < WIFI_RETRY_INTERVAL_MS) {
    return;
  }

  _lastRetryMs = millis();
  Serial.println("Retry WiFi connection");
#if defined(ESP8266)
  WiFi.reconnect();
#else
  WiFi.disconnect();
  if (WiFi.SSID().length() > 0) {
    WiFi.begin();
  } else {
    WiFi.persistent(false);
    WiFi.begin(ssid.c_str(), password.c_str());
    WiFi.persistent(true);
  }
#endif
}

IPAddress WifiManager::dnsAddress() { return WiFi.dnsIP(); }

bool WifiManager::isConnected() { return WiFi.status() == WL_CONNECTED; }

String WifiManager::makeAccessPointName() const {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();
  return "Pnks-" + mac;
}
