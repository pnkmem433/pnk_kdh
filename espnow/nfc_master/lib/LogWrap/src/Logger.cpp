#include "Logger.h"

namespace Logger {

static void appendFormatted(String &out, char spec, const Argument &arg, int width, bool zeroPad) {
  char buf[32];
  if (spec == 's') {
    if (arg.type == Argument::Str && arg.s) {
      out += arg.s;
    } else {
      snprintf(buf, sizeof(buf), "%d", arg.i);
      out += buf;
    }
    return;
  }

  if (arg.type == Argument::Str && arg.s) {
    out += arg.s;
    return;
  }

  const char *fmt = "%d";
  if (spec == 'u') fmt = "%u";
  if (spec == 'x') fmt = "%x";
  if (spec == 'X') fmt = "%X";
  if (width > 0) {
    char fmtBuf[8];
    snprintf(fmtBuf, sizeof(fmtBuf), "%%%s%d%c", zeroPad ? "0" : "", width, spec);
    snprintf(buf, sizeof(buf), fmtBuf, arg.i);
  } else {
    snprintf(buf, sizeof(buf), fmt, arg.i);
  }
  out += buf;
}

void print(const Print &print) {
  if (!print.value) {
    return;
  }
  String out;
  auto it = print.args.begin();
  auto end = print.args.end();

  for (const char *c = print.value; *c; ++c) {
    if (*c == '%' && *(c + 1)) {
      const char *p = c + 1;
      bool zeroPad = false;
      int width = 0;
      if (*p == '0') {
        zeroPad = true;
        ++p;
      }
      while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        ++p;
      }
      char spec = *p;
      if (spec == 's' || spec == 'd' || spec == 'u' || spec == 'x' || spec == 'X') {
        if (it != end) {
          appendFormatted(out, spec, *it, width, zeroPad);
          ++it;
        }
        c = p;
        continue;
      }
    }
    out += *c;
  }

  Serial.printf("%s\n", out.c_str());
}

} // namespace Logger
