/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - Ethernet mutex 보호 구간과 DNS/링크 처리 구현.
*/
#include "lan.h"

#ifndef LAN_LINK_WAIT_MS
#define LAN_LINK_WAIT_MS 15000UL
#endif

#ifndef LAN_LINK_POLL_INTERVAL_MS
#define LAN_LINK_POLL_INTERVAL_MS 250UL
#endif

#ifndef LAN_DHCP_RETRY_COUNT
#define LAN_DHCP_RETRY_COUNT 1
#endif

LAN::LAN(uint8_t cs_pin) : cs_pin(cs_pin), _ethernetMutex(NULL) {}

bool LAN::begin() {
  if (_ethernetMutex == NULL) {
    _ethernetMutex = xSemaphoreCreateMutex();
  }

  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);
  delay(10);

  UIPEthernet.init(cs_pin);

  uint64_t chipId = ESP.getEfuseMac();
  byte mac[6] = {
      0x02,
      static_cast<uint8_t>((chipId >> 40) & 0xFF),
      static_cast<uint8_t>((chipId >> 32) & 0xFF),
      static_cast<uint8_t>((chipId >> 24) & 0xFF),
      static_cast<uint8_t>((chipId >> 16) & 0xFF),
      static_cast<uint8_t>((chipId >> 8) & 0xFF),
  };

  EthernetHardwareStatus hw = UIPEthernet.hardwareStatus();
  if (hw == EthernetNoHardware) {
    xSemaphoreGive(_ethernetMutex);
    return false;
  }

  if (!waitForLink()) {
    xSemaphoreGive(_ethernetMutex);
    return false;
  }

  for (uint8_t attempt = 1; attempt <= LAN_DHCP_RETRY_COUNT; ++attempt) {
    int dhcpResult = UIPEthernet.begin(mac);

    if (dhcpResult == 1) {
      xSemaphoreGive(_ethernetMutex);
      return true;
    }

    if (attempt < LAN_DHCP_RETRY_COUNT) {
      delay(500);
    }
  }

  xSemaphoreGive(_ethernetMutex);
  return false;
}

IPAddress LAN::getLocalIP() { return Ethernet.localIP(); }

IPAddress LAN::resolveDomain(String domain) {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return IPAddress(0, 0, 0, 0);
  }

  IPAddress directIP;
  if (directIP.fromString(domain)) {
    xSemaphoreGive(_ethernetMutex);
    return directIP;
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  DNSClient dns;
  IPAddress dnsIP = IPAddress(8, 8, 8, 8);
  dns.begin(dnsIP);

  IPAddress out;
  int ok = dns.getHostByName(domain.c_str(), out);

  vTaskDelay(pdMS_TO_TICKS(100));
  xSemaphoreGive(_ethernetMutex);

  if (ok == 1) {
    return out;
  }

  return IPAddress(0, 0, 0, 0);
}

bool LAN::isConnected() {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  EthernetLinkStatus status = UIPEthernet.linkStatus();
  xSemaphoreGive(_ethernetMutex);

  return status == LinkON;
}

bool LAN::withEthernet(std::function<bool(EthernetClient &)> callback) {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  EthernetClient client;
  bool result = callback(client);

  xSemaphoreGive(_ethernetMutex);

  return result;
}

bool LAN::waitForLink() {
  unsigned long waitStart = millis();
  EthernetLinkStatus link = UIPEthernet.linkStatus();

  while (link != LinkON && (millis() - waitStart) < LAN_LINK_WAIT_MS) {
    delay(LAN_LINK_POLL_INTERVAL_MS);
    link = UIPEthernet.linkStatus();
  }

  return link == LinkON;
}
