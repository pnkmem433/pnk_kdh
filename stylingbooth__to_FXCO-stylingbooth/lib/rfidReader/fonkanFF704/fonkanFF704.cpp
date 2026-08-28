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
