#include "Uuid.h"

Uuid::Uuid() {}

void Uuid::begin() {
  EEPROM.begin(EEPROM_SIZE);
  delay(100);
  char uuidc[UUID_LENGTH + 1];
  for (int i = 0; i < UUID_LENGTH; i++) {
    uuidc[i] = EEPROM.read(UUID_ADDR + i);
  }
  uuidc[UUID_LENGTH] = '\0';

  uuid = String(uuidc);

  if (!isUuid()) {
    uuid = generator();
    saveUUID();
  }
}

String Uuid::load() { return uuid; }

String Uuid::generator() {
  uint8_t b[16];
  esp_fill_random(b, sizeof(b)); // HW RNG

  b[6] = (b[6] & 0x0F) | 0x40; // version 4
  b[8] = (b[8] & 0x3F) | 0x80; // variant 10xx

  char s[37];
  snprintf(
      s, sizeof(s),
      "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
      b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
      b[12], b[13], b[14], b[15]);
  return String(s);
}

bool Uuid::isUuid() {
  if (uuid.length() != UUID_LENGTH)
    return false;
  for (int i = 0; i < uuid.length(); i++) {
    char c = uuid.charAt(i);
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-')
        return false;
    } else {
      if (!isHexadecimalDigit(c))
        return false;
    }
  }
  return true;
}

void Uuid::saveUUID() {
  for (int i = 0; i < UUID_LENGTH; i++) {
    EEPROM.write(UUID_ADDR + i, uuid.charAt(i));
  }
  EEPROM.commit();
}
