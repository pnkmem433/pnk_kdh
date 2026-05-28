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
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

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
  // 1. NVS에 저장된 WiFi 접속 정보가 있는지 확인하고 우선 연결
  if (WiFi.SSID().length() > 0) {
    Serial.print("Connecting to saved WiFi: ");
    Serial.println(WiFi.SSID());
    WiFi.begin(); // 매개변수 없이 호출하면 저장된 정보로 접속 시도
    
    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      if (tick) tick();
      delay(100);
      Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
  }

  // 2. 저장된 정보가 없거나 연결에 실패하면 Fallback(AppConfig) 자격 증명으로 시도
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

  WiFi.mode(WIFI_AP_STA);
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
  WiFi.disconnect();
  
  // 현재 저장된 WiFi가 있으면 그것으로, 없으면 Fallback으로 재연결
  if (WiFi.SSID().length() > 0) {
    WiFi.begin();
  } else {
    WiFi.begin(ssid.c_str(), password.c_str());
  }
}

IPAddress WifiManager::dnsAddress() { return WiFi.dnsIP(); }

bool WifiManager::isConnected() { return WiFi.status() == WL_CONNECTED; }

String WifiManager::makeAccessPointName() const {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();
  return "Pnks-" + mac;
}
