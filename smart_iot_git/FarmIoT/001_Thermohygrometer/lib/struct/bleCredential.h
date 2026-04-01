#ifndef BLE_CREDENTIAL_STRUCT
#define BLE_CREDENTIAL_STRUCT

#include <Arduino.h>

struct BleCredential {
  String name;
  String uuid;
  String rxChar;
  String txChar;
};

#endif