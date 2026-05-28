#include "fonkanFF704.h"

#include <ctype.h>

FonkanFF704RfidReader::FonkanFF704RfidReader(HardwareSerial &serial,
                                             int8_t rxPin, int8_t txPin,
                                             uint32_t baudrate)
    : serial(serial), baudRate(baudrate), taskHandle1(nullptr),
      taskHandle2(nullptr), txPin(txPin), rxPin(rxPin) {}

void FonkanFF704RfidReader::begin() {
  serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
  serial.setTimeout(30);
}

void FonkanFF704RfidReader::startRead() {
  readingActive = true;
  pollSentCount = 0;
  uartRxCount = 0;
  parsedTagCount = 0;
  shortFrameCount = 0;

  if (taskHandle1 == nullptr) {
    xTaskCreate(
        [](void *pv) {
          FonkanFF704RfidReader *reader =
              static_cast<FonkanFF704RfidReader *>(pv);

          uint64_t pretime = esp_timer_get_time() / 1000;

          while (true) {
            uint64_t nowtime = esp_timer_get_time() / 1000;
            if (nowtime - pretime >= 100) {
              pretime += 100;
              reader->serial.write(reader->readCommand, 3);
              reader->pollSentCount++;
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
        },
        "ff704_task_send_command", 4096, this, 2, &taskHandle1);
  }

  if (taskHandle2 == nullptr) {
    xTaskCreate(
        [](void *pv) {
          FonkanFF704RfidReader *reader =
              static_cast<FonkanFF704RfidReader *>(pv);

          while (true) {
            if (reader->serial.available()) {
              reader->uartRxCount++;
              String tag = reader->serial.readStringUntil(0x0A);
              tag.trim();

              if (tag.length() > 3) {
                reader->parsedTagCount++;
                if (reader->callback != nullptr) {
                  reader->callback(tag);
                }
                reader->tags[tag]++;
              } else {
                reader->shortFrameCount++;
              }
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
        },
        "ff704_task_read_command", 4096, this, 1, &taskHandle2);
  }
}

void FonkanFF704RfidReader::stopRead() {
  readingActive = false;

  if (taskHandle1 != nullptr) {
    vTaskDelete(taskHandle1);
    taskHandle1 = nullptr;
  }

  if (taskHandle2 != nullptr) {
    vTaskDelete(taskHandle2);
    taskHandle2 = nullptr;
  }

  while (serial.available()) {
    serial.read();
  }
}

void FonkanFF704RfidReader::clearTags() { tags.clear(); }

std::map<String, int> FonkanFF704RfidReader::getTags() { return tags; }

void FonkanFF704RfidReader::onRead(std::function<void(String)> cb) {
  callback = cb;
}

String FonkanFF704RfidReader::readerName() { return "Fonkan FF-704"; }

bool FonkanFF704RfidReader::sendAsciiCommandAndReadReply(const String &command,
                                                         String &reply,
                                                         uint32_t timeoutMs) {
  reply = "";

  if (taskHandle1 != nullptr || taskHandle2 != nullptr) {
    return false;
  }

  while (serial.available()) {
    serial.read();
  }

  serial.print(command);
  serial.write('\r');
  serial.flush();

  const uint32_t startMs = millis();
  bool started = false;

  while ((millis() - startMs) < timeoutMs) {
    while (serial.available()) {
      const char ch = static_cast<char>(serial.read());

      if (!started) {
        if (ch == '\n' || ch == '\r') {
          continue;
        }
        started = true;
        reply += ch;
        continue;
      }

      if (ch == '\r') {
        return reply.length() > 0;
      }

      if (ch != '\n') {
        reply += ch;
      }
    }

    delay(1);
  }

  return false;
}

bool FonkanFF704RfidReader::getPowerLevelDbm(int &outDbm) {
  String reply;
  if (!sendAsciiCommandAndReadReply("N0,00", reply)) {
    return false;
  }

  reply.trim();
  if (!reply.startsWith("N") || reply.length() < 2) {
    return false;
  }

  String valueText = reply.substring(1);
  char *end = nullptr;
  long index = strtol(valueText.c_str(), &end, 16);
  if (end == nullptr || *end != '\0' || index < 0x00 || index > 0x1B) {
    return false;
  }

  outDbm = static_cast<int>(index) - 2;
  return true;
}

bool FonkanFF704RfidReader::setPowerLevelDbm(int dbm) {
  if (dbm < -2 || dbm > 25 || taskHandle1 != nullptr || taskHandle2 != nullptr) {
    return false;
  }

  char command[8] = {};
  snprintf(command, sizeof(command), "N1,%02X", dbm + 2);

  while (serial.available()) {
    serial.read();
  }

  serial.print(command);
  serial.write('\r');
  serial.flush();
  delay(50);

  while (serial.available()) {
    serial.read();
  }

  return true;
}
