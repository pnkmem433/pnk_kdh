#pragma once

#include <Arduino.h>

class UuidStore {
public:
  void begin();
  String load() const;

private:
  String generate() const;
  bool isValidUuid(const String& value) const;
  void save(const String& value);

  String uuid_;
};
