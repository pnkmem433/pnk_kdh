#ifndef UUID_H
#define UUID_H

#include <Arduino.h>

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

  static constexpr int UUID_LENGTH = 12;
};

#endif
