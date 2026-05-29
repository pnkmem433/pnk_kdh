#include <map>
#include <vector>

#include <UIPEthernet.h>

#include "doorApi.h"
#include "http.h"
#include "lan.h"
#include "led.h"
#include "rfidReader.h"
#include "sensor.h"

#ifndef API_HOST
#error "API_HOST is not defined. Set -DAPI_HOST=\\\"...\\\" in platformio.ini"
#endif
#ifndef API_PORT
#error "API_PORT is not defined. Set -DAPI_PORT=<int> in platformio.ini"
#endif

#ifndef PIR_SENSOR_PIN
#define PIR_SENSOR_PIN D0
#endif
#ifndef FITTINGROOM_DOOR_PIN
#define FITTINGROOM_DOOR_PIN D1
#endif
#ifndef SHOWROOM_DOOR_PIN
#define SHOWROOM_DOOR_PIN D2
#endif
#ifndef LED_PIN
#define LED_PIN D3
#endif
#ifndef RESET_BUTTON_PIN
#define RESET_BUTTON_PIN D4
#endif
#ifndef LAN_CS_PIN
#define LAN_CS_PIN D5
#endif
#ifndef RFID_RX_PIN
#define RFID_RX_PIN D7
#endif
#ifndef RFID_TX_PIN
#define RFID_TX_PIN D6
#endif
#ifndef RFID_BAUD_RATE
#define RFID_BAUD_RATE 115200
#endif
#ifndef RFID_DEFAULT_POWER_DBM
#define RFID_DEFAULT_POWER_DBM -2
#endif
#ifndef FITTING_ROOM_ID
#define FITTING_ROOM_ID 1
#endif
#ifndef RFID_MIN_READ_COUNT
#define RFID_MIN_READ_COUNT 5
#endif
#ifndef RFID_TAG_SUBSTRING_START
#define RFID_TAG_SUBSTRING_START 5
#endif
#ifndef RFID_TAG_SUBSTRING_END
#define RFID_TAG_SUBSTRING_END 13
#endif

namespace {
constexpr int kRfidPowerMinDbm = -2;
constexpr int kRfidPowerMaxDbm = 25;
constexpr uint64_t kReadingWindowUs = 10ULL * 1000ULL * 1000ULL;

LAN lan(LAN_CS_PIN);
LED led = LED(LED_PIN);

Sensor pirSensor = Sensor(PIR_SENSOR_PIN);
Sensor fittingroomDoor = Sensor(FITTINGROOM_DOOR_PIN, INPUT_PULLUP);
Sensor showroomDoor = Sensor(SHOWROOM_DOOR_PIN, INPUT_PULLDOWN);
Sensor resetButton = Sensor(RESET_BUTTON_PIN);

RfidReader *reader =
    new FonkanFF704RfidReader(Serial1, RFID_RX_PIN, RFID_TX_PIN, RFID_BAUD_RATE);

volatile bool ethernetReady = false;
volatile uint64_t lastCloseTimeUs = 0;
volatile bool rfidReading = false;

bool lastPirState = false;
bool doorClosedStateKnown = false;
bool doorClosedState = false;

bool withDoorApi(const std::function<void(DoorApi &)> &callback) {
  if (!ethernetReady) {
    return false;
  }

  return lan.withEthernet([&](EthernetClient &client) {
    Http http(&client);
    DoorApi door(&http, API_HOST, API_PORT);
    http.begin();
    door.begin();
    door.setFittingRoomId(FITTING_ROOM_ID);
    callback(door);
    return true;
  });
}

void connectLanOnce() {
  Serial.println("Connecting to LAN...");
  ethernetReady = lan.begin();

  if (ethernetReady) {
    Serial.println("LAN connected!");
    Serial.print("IP: ");
    Serial.println(lan.getLocalIP());
  } else {
    Serial.println("LAN not connected. Local sensing stays active.");
  }
}

void applyConfiguredRfidPower() {
  if (RFID_DEFAULT_POWER_DBM < kRfidPowerMinDbm ||
      RFID_DEFAULT_POWER_DBM > kRfidPowerMaxDbm) {
    Serial.printf("RFID default power out of range: %d dBm\n",
                  RFID_DEFAULT_POWER_DBM);
    return;
  }

  if (reader->setPowerLevelDbm(RFID_DEFAULT_POWER_DBM)) {
    Serial.printf("RFID default power set: %d dBm\n", RFID_DEFAULT_POWER_DBM);
  } else {
    Serial.printf("RFID default power set failed: %d dBm\n",
                  RFID_DEFAULT_POWER_DBM);
  }
}

void stopReadingAndClear() {
  if (rfidReading) {
    reader->stopRead();
    rfidReading = false;
  }
  reader->clearTags();
}

void startReadingWindow() {
  reader->clearTags();
  reader->startRead();
  rfidReading = true;
  lastCloseTimeUs = esp_timer_get_time();
}

void printTagSummary(const std::map<String, int> &tags) {
  if (tags.empty()) {
    Serial.println("RFID scan complete: no tags");
    return;
  }

  Serial.printf("RFID scan complete: %u unique tags\n",
                static_cast<unsigned>(tags.size()));
  for (const auto &entry : tags) {
    Serial.printf("RFID summary %s count=%d\n", entry.first.c_str(),
                  entry.second);
  }
}

std::vector<String> collectQualifiedTags(const std::map<String, int> &tags) {
  std::vector<String> tagList;

  for (const auto &entry : tags) {
    if (entry.second < RFID_MIN_READ_COUNT ||
        entry.first.length() < RFID_TAG_SUBSTRING_END) {
      continue;
    }

    tagList.push_back(
        entry.first.substring(RFID_TAG_SUBSTRING_START, RFID_TAG_SUBSTRING_END));
  }

  return tagList;
}

void handleDoorOpen() {
  if (doorClosedStateKnown && !doorClosedState) {
    return;
  }

  Serial.println("Door open");
  stopReadingAndClear();
  led.on();
  doorClosedStateKnown = true;
  doorClosedState = false;

  if (!withDoorApi([](DoorApi &door) { door.openDoor(); })) {
    Serial.println("Door open sync skipped: LAN unavailable");
  }
}

void handleDoorClosed() {
  if (doorClosedStateKnown && doorClosedState) {
    return;
  }

  Serial.println("Door closed");
  startReadingWindow();
  led.blink(100, 100);
  doorClosedStateKnown = true;
  doorClosedState = true;

  if (!withDoorApi([](DoorApi &door) { door.closeDoor(); })) {
    Serial.println("Door closed sync skipped: LAN unavailable");
  }
}

void setupDoorSensors() {
  fittingroomDoor.begin({
      {.event = Event::Low,
       .delayMs = NO_DELAY,
       .action = []() { handleDoorOpen(); }},
      {.event = Event::High,
       .delayMs = NO_DELAY,
       .action = []() { handleDoorClosed(); }},
  });

  showroomDoor.begin({
      {.event = Event::Low,
       .delayMs = NO_DELAY,
       .action = []() { Serial.println("Showroom door open"); }},
      {.event = Event::High,
       .delayMs = NO_DELAY,
       .action = []() { Serial.println("Showroom door closed"); }},
  });
}

void startRfidFinalizeTask() {
  xTaskCreate(
      [](void *param) {
        while (1) {
          if (lastCloseTimeUs != 0) {
            const uint64_t now = esp_timer_get_time();
            if (now - lastCloseTimeUs >= kReadingWindowUs) {
              reader->stopRead();
              rfidReading = false;

              std::map<String, int> tags = reader->getTags();
              printTagSummary(tags);

              std::vector<String> tagList = collectQualifiedTags(tags);
              if (!tagList.empty()) {
                if (withDoorApi(
                        [&](DoorApi &door) { door.sendTags(tagList); })) {
                  Serial.printf("RFID sent %u items\n",
                                static_cast<unsigned>(tagList.size()));
                } else {
                  Serial.println("RFID send skipped: LAN unavailable");
                }
              } else {
                Serial.println("RFID send skipped: no qualified tags");
              }

              reader->clearTags();
              led.on();
              lastCloseTimeUs = 0;
            }
          }

          vTaskDelay(pdMS_TO_TICKS(100));
        }
      },
      "RfidFinalizeTask", 4096, NULL, 1, NULL);
}

void handleSerialCommands() {
  static String commandBuffer;

  while (Serial.available()) {
    char ch = static_cast<char>(Serial.read());
    if (ch == '\r') {
      continue;
    }

    if (ch != '\n') {
      commandBuffer += ch;
      continue;
    }

    String line = commandBuffer;
    commandBuffer = "";
    line.trim();
    line.toUpperCase();

    if (line == "RFID POWER_GET") {
      int dbm = 0;
      if (rfidReading) {
        Serial.println("RFID power busy");
      } else if (reader->getPowerLevelDbm(dbm)) {
        Serial.printf("RFID power %d dBm\n", dbm);
      } else {
        Serial.println("RFID power unknown");
      }
      continue;
    }

    if (line.startsWith("RFID POWER_SET ")) {
      const String valueText = line.substring(String("RFID POWER_SET ").length());
      const int dbm = valueText.toInt();

      if (rfidReading) {
        Serial.println("RFID power busy");
      } else if (dbm < kRfidPowerMinDbm || dbm > kRfidPowerMaxDbm) {
        Serial.printf("RFID power range %d..%d dBm\n", kRfidPowerMinDbm,
                      kRfidPowerMaxDbm);
      } else if (reader->setPowerLevelDbm(dbm)) {
        Serial.printf("RFID power set %d dBm\n", dbm);
      } else {
        Serial.println("RFID power set failed");
      }
    }
  }
}
} // namespace

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("System boot");

  led.begin();
  led.blink(500, 500);

  reader->onRead([](String tag) {
    Serial.printf("RFID read %s\n", tag.c_str());
  });
  reader->begin();
  reader->stopRead();
  applyConfiguredRfidPower();

  connectLanOnce();
  setupDoorSensors();
  startRfidFinalizeTask();

  led.on();
  Serial.printf("RFID default power config: %d dBm\n", RFID_DEFAULT_POWER_DBM);
  Serial.println("Power command: RFID POWER_GET / RFID POWER_SET <dbm>");
}

void loop() {
  const bool currentPirState = digitalRead(PIR_SENSOR_PIN) == HIGH;
  if (currentPirState != lastPirState) {
    lastPirState = currentPirState;
    Serial.printf("PIR %s\n", currentPirState ? "motion" : "idle");
  }

  handleSerialCommands();
  delay(10);
}
