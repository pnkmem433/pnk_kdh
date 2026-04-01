/*
  [파일 설명] lib/wifi/wifiManager.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "WifiManager.h"

WifiManager *WifiManager::instance = nullptr;

WifiManager::WifiManager(std::vector<WifiInfo> wifiInfos)
    : wifiInfos(std::move(wifiInfos)) {}

// begin(): Wi-Fi 스택 시작 + 초기 연결 + 재연결 태스크 가동
bool WifiManager::begin(String name) {
  // Hostname은 공유기 관리 화면/네트워크 스캔 시 장치 식별에 도움 된다.
  if (name.length()) {
    WiFi.setHostname(name.c_str());
  }

  // ESP 이벤트 핸들러가 인스턴스로 되돌아올 수 있게 현재 객체를 보관한다.
  instance = this;

  // STA 모드 강제 + 절전 비활성화(연결 지연/패킷 드랍 완화)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  // 이전 연결 상태/설정을 초기화하고 깨끗하게 시작한다.
  WiFi.disconnect(true, true);
  delay(100);

  // WiFiMulti에 연결 후보 AP들을 등록한다.
  for (auto &wifi : wifiInfos) {
    wifiMulti.addAP(wifi.ssid.c_str(), wifi.password.c_str());
  }

  // 이벤트 등록 + SDK 자동 재연결 활성화
  WiFi.onEvent(WiFiEventHandler);
  WiFi.setAutoReconnect(true);

  // --------------------------------------------------------------------------
  // 초기 블로킹 연결 구간
  // - 부팅 직후 연결 성공률을 높이기 위해 짧게 집중 시도
  // - 단, 영원히 대기하지 않도록 10초 타임아웃 적용
  // - 타임아웃 후에도 시스템은 계속 동작(후속 태스크에서 재연결)
  // --------------------------------------------------------------------------
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 10000) {
      Serial.println("[WiFi] Timeout! Proceeding without connection...");
      break;
    }
    wifiMulti.run();
    delay(200);
  }

  Serial.printf("[WiFi] connected to %s, IP=%s\n", WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str());

  // 재연결 감시 태스크를 1회만 만든다.
  // 이 태스크는 장치 생명주기 동안 계속 살아있으며, 끊김 복구를 담당한다.
  if (wifiTaskHandle == nullptr) {
    xTaskCreate(wifiTask, "WifiTask", 4096, this, 1, &wifiTaskHandle);
  }

  return true;
}

// wifiTask(): 연결이 끊긴 경우에만 WiFiMulti.run()으로 재시도
void WifiManager::wifiTask(void *param) {
  auto *self = static_cast<WifiManager *>(param);

  for (;;) {
    // 이미 연결된 상태에서 run()을 과도하게 호출하면 불필요한 부하가 생길 수 있다.
    // 따라서 "끊어진 경우"에만 run()을 호출해 재연결을 시도한다.
    if (WiFi.status() != WL_CONNECTED) {
      self->wifiMulti.run();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// WiFiEventHandler(): static 이벤트 핸들러 -> instance 전달 브릿지
void WifiManager::WiFiEventHandler(arduino_event_id_t event,
                                   arduino_event_info_t info) {
  (void)info;
  if (instance) {
    instance->handleWiFiEvent(event);
  }
}

// handleWiFiEvent(): 연결/해제 이벤트를 상위 콜백에 전달
void WifiManager::handleWiFiEvent(arduino_event_id_t event) {
  switch (event) {
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    // 연결 콜백은 연결 직후 IP 할당 타이밍과 완전히 일치하지 않을 수 있으나,
    // 현재 코드에서는 즉시 getConnectedInfo()를 전달한다.
    if (connectCallback) {
      ConnectedInfo info = getConnectedInfo();
      connectCallback(info);
    }
    break;

  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    // 해제 시 콜백에서 재시도 UI 표시/로그 기록 등을 처리할 수 있다.
    if (disconnectCallback) {
      disconnectCallback();
    }
    break;

  default:
    break;
  }
}

bool WifiManager::isConnected() { return WiFi.status() == WL_CONNECTED; }

IPAddress WifiManager::dnsAddress() { return WiFi.dnsIP(); }

void WifiManager::onConnect(std::function<void(ConnectedInfo)> cb) {
  connectCallback = std::move(cb);
}

void WifiManager::onDisconnect(std::function<void()> cb) {
  disconnectCallback = std::move(cb);
}

ConnectedInfo WifiManager::getConnectedInfo() {
  ConnectedInfo info;
  info.ssid = WiFi.SSID();
  info.ip = WiFi.localIP();
  return info;
}

int32_t WifiManager::rssi() { return WiFi.RSSI(); }

