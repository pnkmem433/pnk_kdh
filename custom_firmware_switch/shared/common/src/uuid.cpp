#include "uuid.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

Uuid::Uuid() {}

void Uuid::begin() {
  EEPROM.begin(EEPROM_SIZE);
  delay(100);

  char uuidc[UUID_LENGTH + 1];
  for (int i = 0; i < UUID_LENGTH; ++i) {
    uuidc[i] = EEPROM.read(UUID_ADDR + i);
  }
  uuidc[UUID_LENGTH] = '\0';

  uuid = String(uuidc);
  uuid.trim();

  if (!isUuid()) {
    uuid = generator();
    saveUUID();
  }
}

String Uuid::load() { return uuid; }

String Uuid::generator() {
#if defined(ESP8266)
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char s[UUID_LENGTH + 1];
  // MQTT에서 보기 쉽게 하이픈 없는 12자리 MAC 문자열만 사용
  snprintf(s, sizeof(s), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
  return String(s);
#else
  const uint64_t mac = ESP.getEfuseMac();
  char s[UUID_LENGTH + 1];
  // eFuse MAC의 마지막 12자리만 UUID로 사용
  snprintf(s, sizeof(s), "%012llX",
           static_cast<unsigned long long>(mac & 0xFFFFFFFFFFFFULL));
  return String(s);
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
  for (int i = 0; i < UUID_LENGTH; ++i) {
    EEPROM.write(UUID_ADDR + i, uuid.charAt(i));
  }
  EEPROM.commit();
}
