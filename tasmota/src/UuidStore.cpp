#include "UuidStore.h"

#include <EEPROM.h>

namespace {
constexpr int kEepromSize = 64;
constexpr int kUuidAddr = 0;
constexpr int kUuidLength = 36;
}  // namespace

void UuidStore::begin() {
  EEPROM.begin(kEepromSize);

  char buffer[kUuidLength + 1];
  for (int i = 0; i < kUuidLength; ++i) {
    buffer[i] = static_cast<char>(EEPROM.read(kUuidAddr + i));
  }
  buffer[kUuidLength] = '\0';

  String candidate(buffer);
  if (isValidUuid(candidate)) {
    uuid_ = candidate;
    return;
  }

  uuid_ = generate();
  save(uuid_);
}

String UuidStore::load() const { return uuid_; }

String UuidStore::generate() const {
  uint8_t randomBytes[16];
  for (size_t i = 0; i < sizeof(randomBytes); ++i) {
    randomBytes[i] = static_cast<uint8_t>(random(0, 256));
  }

  randomBytes[6] = (randomBytes[6] & 0x0F) | 0x40;
  randomBytes[8] = (randomBytes[8] & 0x3F) | 0x80;

  char out[37];
  snprintf(out, sizeof(out),
           "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
           randomBytes[0], randomBytes[1], randomBytes[2], randomBytes[3],
           randomBytes[4], randomBytes[5], randomBytes[6], randomBytes[7],
           randomBytes[8], randomBytes[9], randomBytes[10], randomBytes[11],
           randomBytes[12], randomBytes[13], randomBytes[14], randomBytes[15]);
  return String(out);
}

bool UuidStore::isValidUuid(const String& value) const {
  if (value.length() != kUuidLength) {
    return false;
  }

  for (unsigned int i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-') {
        return false;
      }
      continue;
    }
    if (!isHexadecimalDigit(c)) {
      return false;
    }
  }
  return true;
}

void UuidStore::save(const String& value) {
  for (int i = 0; i < kUuidLength; ++i) {
    EEPROM.write(kUuidAddr + i, value.charAt(i));
  }
  EEPROM.commit();
}
