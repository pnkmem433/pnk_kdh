#ifndef UUID_H
#define UUID_H

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE 64
#define UUID_LENGTH 36
#define UUID_ADDR 0

class Uuid {
public:
  Uuid();
  void begin();
  String load();

private:
  String generator();
  bool isUuid();
  void saveUUID();
  String uuid;
};

#endif
