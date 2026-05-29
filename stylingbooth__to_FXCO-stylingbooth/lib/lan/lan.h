#ifndef LAN_H
#define LAN_H

#include "UIPEthernet.h"
#include "arduino.h"
#include "functional"
#include <Dns.h>
#include <mutex>

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
  void printMacAddress(const uint8_t *mac);
};

#endif
