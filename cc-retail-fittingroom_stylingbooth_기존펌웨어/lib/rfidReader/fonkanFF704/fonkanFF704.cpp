#include "fonkanFF704.h"
#include <ctype.h>

namespace {
constexpr uint64_t RFID_POLL_INTERVAL_MS = 50;
constexpr uint32_t RFID_SERIAL_TIMEOUT_MS = 30;

String normalizeTag(String raw) {
  raw.trim();

  String cleaned = "";
  for (size_t i = 0; i < raw.length(); ++i) {
    unsigned char c = static_cast<unsigned char>(raw[i]);
    if (!isprint(c) || isspace(c)) {
      continue;
    }
    cleaned += static_cast<char>(toupper(c));
  }

  if (cleaned.length() != 33 || cleaned[0] != 'U') {
    return "";
  }

  for (size_t i = 1; i < cleaned.length(); ++i) {
    char c = cleaned[i];
    bool isHex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
    if (!isHex) {
      return "";
    }
  }

  return cleaned;
}
} // namespace

FonkanFF704RfidReader::FonkanFF704RfidReader(HardwareSerial &serial,
                                             int8_t rxPin, int8_t txPin,
                                             uint32_t baudrate)
    : serial(serial), baudRate(baudrate), callback(nullptr), rxPin(rxPin),
      txPin(txPin), taskHandle1(nullptr), taskHandle2(nullptr),
      tagsMutex(nullptr), serialMutex(nullptr), readingEnabled(false) {}

void FonkanFF704RfidReader::begin() {
  serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
  serial.setTimeout(RFID_SERIAL_TIMEOUT_MS);

  if (tagsMutex == nullptr) {
    tagsMutex = xSemaphoreCreateMutex();
  }
  if (serialMutex == nullptr) {
    serialMutex = xSemaphoreCreateMutex();
  }
}

void FonkanFF704RfidReader::startRead() {
  readingEnabled = true;
  while (serial.available()) {
    serial.read();
  }

  if (taskHandle1 == nullptr) {
    xTaskCreate(
        [](void *pv) {
          auto *reader = static_cast<FonkanFF704RfidReader *>(pv);
          uint64_t pretime = esp_timer_get_time() / 1000;

          while (true) {
            if (!reader->readingEnabled) {
              pretime = esp_timer_get_time() / 1000;
              vTaskDelay(pdMS_TO_TICKS(10));
              continue;
            }

            uint64_t nowtime = esp_timer_get_time() / 1000;
            if (nowtime - pretime >= RFID_POLL_INTERVAL_MS) {
              pretime += RFID_POLL_INTERVAL_MS;
              reader->serial.write(reader->readCommand, 3);
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
        },
        "ff704_task_send_command", 4096, this, 2, &taskHandle1);
  }

  if (taskHandle2 == nullptr) {
    xTaskCreate(
        [](void *pv) {
          auto *reader = static_cast<FonkanFF704RfidReader *>(pv);

          while (true) {
            if (!reader->readingEnabled) {
              while (reader->serial.available()) {
                reader->serial.read();
              }
              vTaskDelay(pdMS_TO_TICKS(10));
              continue;
            }

            if (reader->serial.available()) {
              String raw = reader->serial.readStringUntil(0x0A);
              String tag = normalizeTag(raw);

              if (tag.length() > 0) {
                if (reader->callback != nullptr) {
                  reader->callback(tag);
                }
                if (reader->tagsMutex != nullptr &&
                    xSemaphoreTake(reader->tagsMutex, portMAX_DELAY) == pdTRUE) {
                  reader->tags[tag]++;
                  xSemaphoreGive(reader->tagsMutex);
                }
              }
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
        },
        "ff704_task_read_command", 4096, this, 1, &taskHandle2);
  }
}

void FonkanFF704RfidReader::stopRead() {
  readingEnabled = false;
  while (serial.available()) {
    serial.read();
  }
}

void FonkanFF704RfidReader::clearTags() {
  if (tagsMutex != nullptr &&
      xSemaphoreTake(tagsMutex, portMAX_DELAY) == pdTRUE) {
    tags.clear();
    xSemaphoreGive(tagsMutex);
  }
}

std::map<String, int> FonkanFF704RfidReader::getTags() {
  std::map<String, int> snapshot;
  if (tagsMutex != nullptr &&
      xSemaphoreTake(tagsMutex, portMAX_DELAY) == pdTRUE) {
    snapshot = tags;
    xSemaphoreGive(tagsMutex);
  }
  return snapshot;
}

void FonkanFF704RfidReader::onRead(std::function<void(String)> cb) {
  callback = cb;
}

String FonkanFF704RfidReader::readerName() { return "Fonkan FF-704"; }

bool FonkanFF704RfidReader::sendAsciiCommandAndReadReply(const String &command,
                                                         String &reply,
                                                         uint32_t timeoutMs) {
  reply = "";

  if (serialMutex == nullptr || readingEnabled) {
    return false;
  }

  if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(timeoutMs)) != pdTRUE) {
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
        xSemaphoreGive(serialMutex);
        return reply.length() > 0;
      }

      if (ch != '\n') {
        reply += ch;
      }
    }

    delay(1);
  }

  xSemaphoreGive(serialMutex);
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
  if (dbm < -2 || dbm > 25 || serialMutex == nullptr || readingEnabled) {
    return false;
  }

  char command[8] = {};
  snprintf(command, sizeof(command), "N1,%02X", dbm + 2);

  if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(400)) != pdTRUE) {
    return false;
  }

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

  xSemaphoreGive(serialMutex);
  return true;
}
