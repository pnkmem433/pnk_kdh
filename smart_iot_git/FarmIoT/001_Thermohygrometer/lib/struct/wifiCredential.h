#ifndef WIFI_CREDENTIAL_STRUCT
#define WIFI_CREDENTIAL_STRUCT

#include <Arduino.h>

struct WifiCredential {
  String ssid;
  String password;
};

#endif