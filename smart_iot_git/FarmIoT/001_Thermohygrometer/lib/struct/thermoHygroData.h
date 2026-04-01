#ifndef THERMOHYGROMETER_DATA_STRUCT
#define THERMOHYGROMETER_DATA_STRUCT

#include <Arduino.h>

#include "dateTime.h"

struct ThermohygrometerData {
  DateTime dateTime;
  float temp;
  float humi;

  String toString() {
    return dateTime.toString() + " : " + String(temp) + "c, " + String(humi) +
           "%";
  }
};

#endif