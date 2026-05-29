#include "UidFormat.h"

namespace UidFormat {

void printToSerial(const uint8_t *uid, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    if (uid[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(uid[i], HEX);
    if (i + 1 < len) {
      Serial.print(':');
    }
  }
}

} // namespace UidFormat
