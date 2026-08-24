#include "fonkanFF701.h"

FonkanFF701RfidReader::FonkanFF701RfidReader(HardwareSerial &serial,
                                             int8_t rxPin, int8_t txPin,
                                             uint32_t baudrate)
    : serial(serial), baudRate(baudrate), taskHandle(nullptr), txPin(txPin),
      rxPin(rxPin) {}

void FonkanFF701RfidReader::begin() {
  serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
}

void FonkanFF701RfidReader::startRead() {
  serial.write(startCommand, 5);
  if (taskHandle == nullptr) {
    xTaskCreate(
        [](void *pv) {
          FonkanFF701RfidReader *reader =
              static_cast<FonkanFF701RfidReader *>(pv);

          while (true) {
            if (reader->serial.available()) {
              String data = reader->serial.readStringUntil('\n');
              int index = data.indexOf("Tag:");
              if (index != -1) {
                String tag = data.substring(index + 4);
                tag.trim();
                if (reader->callback != nullptr) {
                  reader->callback(tag);
                }
                reader->tags[tag]++;
              }
            }
            vTaskDelay(2 / portTICK_PERIOD_MS);
          }
        },
        "ff701_task", 4096, this, 1, &taskHandle);
  }
}

void FonkanFF701RfidReader::stopRead() {
  serial.write(stopCommand, 5);
  if (taskHandle != nullptr) {
    vTaskDelete(taskHandle);
    taskHandle = nullptr;
  }
}

void FonkanFF701RfidReader::clearTags() { tags.clear(); }

std::map<String, int> FonkanFF701RfidReader::getTags() { return tags; }

void FonkanFF701RfidReader::onRead(std::function<void(String)> cb) {
  callback = cb;
}

String FonkanFF701RfidReader::readerName() { return "Fonkan FF-701"; }
