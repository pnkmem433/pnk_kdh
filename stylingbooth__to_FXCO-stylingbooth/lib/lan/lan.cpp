#include "lan.h"
#include "esp_mac.h"

#ifndef LAN_LINK_WAIT_MS
#define LAN_LINK_WAIT_MS 15000UL
#endif

#ifndef LAN_LINK_POLL_INTERVAL_MS
#define LAN_LINK_POLL_INTERVAL_MS 250UL
#endif

#ifndef LAN_DHCP_RETRY_COUNT
#define LAN_DHCP_RETRY_COUNT 1
#endif

namespace {
const char *linkStatusToString(EthernetLinkStatus status) {
  switch (status) {
  case LinkON:
    return "LinkON";
  case LinkOFF:
    return "LinkOFF";
  default:
    return "Unknown";
  }
}

const char *hardwareStatusToString(EthernetHardwareStatus status) {
  switch (status) {
  case EthernetNoHardware:
    return "EthernetNoHardware";
  case EthernetW5100:
    return "EthernetW5100-compatible";
  default:
    return "EthernetHardwareDetected";
  }
}
} // namespace

LAN::LAN(uint8_t cs_pin) : cs_pin(cs_pin), _ethernetMutex(NULL) {}

bool LAN::begin() {
  if (_ethernetMutex == NULL) {
    _ethernetMutex = xSemaphoreCreateMutex();
  }

  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);
  delay(10);

  UIPEthernet.init(cs_pin);

  byte mac[6];
  esp_read_mac(mac, ESP_MAC_ETH);

  EthernetHardwareStatus hw = UIPEthernet.hardwareStatus();
  if (hw == EthernetNoHardware) {
    return false;
  }

  if (!waitForLink()) {
    return false;
  }

  for (uint8_t attempt = 1; attempt <= LAN_DHCP_RETRY_COUNT; ++attempt) {
    int dhcpResult = UIPEthernet.begin(mac);

    if (dhcpResult == 1) {
      return true;
    }

    if (attempt < LAN_DHCP_RETRY_COUNT) {
      delay(500);
    }
  }
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

  Serial.println("DNS lookup failed");
  return IPAddress(0, 0, 0, 0);
}

bool LAN::isConnected() {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  vTaskDelay(pdMS_TO_TICKS(100));
  EthernetLinkStatus status = UIPEthernet.linkStatus();
  UIPEthernet.maintain();
  vTaskDelay(pdMS_TO_TICKS(100));

  xSemaphoreGive(_ethernetMutex);

  return status == LinkON;
}

bool LAN::withEthernet(std::function<bool(EthernetClient &)> callback) {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  EthernetClient client;

  vTaskDelay(pdMS_TO_TICKS(100));
  bool result = callback(client);
  vTaskDelay(pdMS_TO_TICKS(100));

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

void LAN::printMacAddress(const uint8_t *mac) {
  Serial.printf("[LAN] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1],
                mac[2], mac[3], mac[4], mac[5]);
}
