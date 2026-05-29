/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - Ethernet 접근 직렬화 API 정의.
*/
#ifndef LAN_H
#define LAN_H

#include "UIPEthernet.h"
#include "arduino.h"
#include "functional"
#include <Dns.h>

class LAN {
public:
  LAN(uint8_t cs_pin);

  bool begin();
  IPAddress getLocalIP();
  IPAddress resolveDomain(String domain);
  bool isConnected();
  bool withEthernet(std::function<bool(EthernetClient &)> callback);

private:
  uint8_t cs_pin;
  SemaphoreHandle_t _ethernetMutex;

  bool waitForLink();
};

#endif
