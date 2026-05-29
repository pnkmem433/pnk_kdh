#include "HexFormat.h"

namespace HexFormat {

String toString(const ToString &config) {
  uint8_t width = config.width == 0 ? 2 : config.width;
  bool upper = config.upper;
  bool prefix = config.prefix;

  char fmt[8];
  if (prefix) {
    snprintf(fmt, sizeof(fmt), "0x%%0%u%c", width, upper ? 'X' : 'x');
  } else {
    snprintf(fmt, sizeof(fmt), "%%0%u%c", width, upper ? 'X' : 'x');
  }

  char buf[16];
  snprintf(buf, sizeof(buf), fmt, static_cast<unsigned int>(config.value));
  if (prefix && upper) {
    buf[1] = 'X';
  }
  return String(buf);
}

} // namespace HexFormat
