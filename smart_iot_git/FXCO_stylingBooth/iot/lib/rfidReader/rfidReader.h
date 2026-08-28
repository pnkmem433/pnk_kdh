#ifndef RFID_READER_H
#define RFID_READER_H

#include "Arduino.h"
#include "functional"
#include <map>

class RfidReader {
public:
  virtual void begin() = 0;
  virtual void startRead() = 0;
  virtual void stopRead() = 0;

  virtual std::map<String, int> getTags() = 0;
  virtual void clearTags() = 0;

  virtual void onRead(std::function<void(String)> callback) = 0;

  virtual String readerName() = 0;
};

#include "fonkanFF701/fonkanFF701.h"
#include "fonkanFF704/fonkanFF704.h"

#endif