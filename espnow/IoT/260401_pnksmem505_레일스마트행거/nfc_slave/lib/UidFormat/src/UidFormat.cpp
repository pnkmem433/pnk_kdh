#include "UidFormat.h"

namespace UidFormat {

void toHexString(char *out, size_t outSize, const uint8_t *uid, uint8_t len) {
  size_t pos = 0;
  for (uint8_t i = 0; i < len && pos + 3 < outSize; i++) {
    uint8_t v = uid[i];
    const char hex[] = "0123456789ABCDEF";
    out[pos++] = hex[(v >> 4) & 0x0F];
    out[pos++] = hex[v & 0x0F];
    if (i + 1 < len && pos + 1 < outSize) {
      out[pos++] = ':';
    }
  }
  out[pos] = '\0';
}

} // namespace UidFormat
