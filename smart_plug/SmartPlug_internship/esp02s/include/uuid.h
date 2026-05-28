#ifndef UUID_H
#define UUID_H

#include <Arduino.h>
#include <EEPROM.h>

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

  static constexpr int EEPROM_SIZE = 32;
  static constexpr int UUID_ADDR = 0;
  static constexpr int UUID_LENGTH = 12;
};

#endif