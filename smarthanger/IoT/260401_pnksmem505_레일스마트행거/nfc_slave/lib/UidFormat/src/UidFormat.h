#pragma once

#include <Arduino.h>

namespace UidFormat {
void toHexString(char *out, size_t outSize, const uint8_t *uid, uint8_t len);
}
