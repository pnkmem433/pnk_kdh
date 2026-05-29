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
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char s[UUID_LENGTH + 1];
  
  // 정순서(mac[0] ~ mac[5])로 수정하여 Tasmota와 동일한 ID 생성
  snprintf(s, sizeof(s), "%02X%02X%02X%02X%02X%02X", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
           
  return String(s);
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