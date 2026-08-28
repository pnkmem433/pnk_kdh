#include "uuid.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

Uuid::Uuid() {}

void Uuid::begin() {
  uuid = generator();
}

String Uuid::load() { return uuid; }

String Uuid::generator() {
  String uniqueId = WiFi.macAddress();
  uniqueId.replace(":", "");
  uniqueId.toUpperCase();

  if (uniqueId.length() == UUID_LENGTH) {
    return uniqueId;
  }

#if defined(ESP8266)
  char fallback[UUID_LENGTH + 1];
  snprintf(fallback, sizeof(fallback), "%012X", ESP.getChipId());
  return String(fallback);
#else
  uint64_t mac = ESP.getEfuseMac();
  char fallback[UUID_LENGTH + 1];
  snprintf(fallback, sizeof(fallback), "%012llX", mac & 0xFFFFFFFFFFFFULL);
  return String(fallback);
#endif
}

bool Uuid::isUuid() {
  if (uuid.length() != UUID_LENGTH) {
    return false;
  }

  for (unsigned int i = 0; i < uuid.length(); ++i) {
    if (!isHexadecimalDigit(uuid.charAt(i))) {
      return false;
    }
  }
  return true;
}

void Uuid::saveUUID() {
  // No-op: dynamically loaded from WiFi MAC address ROM
}