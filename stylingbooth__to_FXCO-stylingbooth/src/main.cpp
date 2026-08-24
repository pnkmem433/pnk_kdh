#include <Arduino.h>
#include "FirmwareUpdater.h"
#include "doorSensor.h"
#include "hexAsciiConverter.h"
#include "httpManageX.h"
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
#ifndef FIRMWARE_HOST
#error "FIRMWARE_HOST is not defined. Set -DFIRMWARE_HOST=\\\"...\\\" in platformio.ini"
#endif
#ifndef FIRMWARE_PORT
#error "FIRMWARE_PORT is not defined. Set -DFIRMWARE_PORT=<int> in platformio.ini"
#endif
#ifndef FIRMWARE_ID
#error "FIRMWARE_ID is not defined. Set -DFIRMWARE_ID=\\\"...\\\" in platformio.ini"
#endif

#ifndef DEFAULT_LOADING_TIME_SECONDS
#define DEFAULT_LOADING_TIME_SECONDS 10
#endif

#ifndef LAN_RECOVERY_INTERVAL_MS
#define LAN_RECOVERY_INTERVAL_MS 15000UL
#endif

Sensor pir = Sensor(D0);
DoorSensor fittingroomDoor = DoorSensor(D1, INPUT_PULLUP);
DoorSensor showroomDoor = DoorSensor(D2, INPUT_PULLDOWN);
Sensor touchBtn = Sensor(D3);
LED led = LED(D4);

LAN lan = LAN(D5);
RfidReader *reader = new FonkanFF704RfidReader(Serial1, D7, D6, 115200);
HttpManageX http = HttpManageX(API_HOST, API_PORT);
FirmwareUpdater updater =
    FirmwareUpdater(FIRMWARE_HOST, FIRMWARE_PORT, FIRMWARE_ID, 15);

QueueHandle_t networkQueue = nullptr;
bool lanReady = false;
bool networkBootstrapDone = false;
bool networkAttemptCompleted = false;
unsigned long lastLanAttemptMs = 0;
volatile uint64_t lastCloseTimeUs = 0;
volatile uint64_t readingTimeUs =
    static_cast<uint64_t>(DEFAULT_LOADING_TIME_SECONDS) * 1000ULL * 1000ULL;

struct PendingNetworkEvent {
  enum Type : uint8_t {
    FittingDoorOpened,
    FittingDoorClosed,
    ShowroomDoorOpened,
    ShowroomDoorClosed,
    SendBestTag,
  } type;
  char payload[64];
};

void enqueueNetworkEvent(PendingNetworkEvent::Type type,
                         const String &payload = "") {
  if (networkQueue == nullptr) {
    return;
  }

  PendingNetworkEvent event = {};
  event.type = type;
  payload.substring(0, sizeof(event.payload) - 1)
      .toCharArray(event.payload, sizeof(event.payload));
  xQueueSend(networkQueue, &event, 0);
}

String selectBestAsciiTag(const std::map<String, int> &tags) {
  int bestCount = 0;
  String bestTag;

  for (const auto &tag : tags) {
    String asciiTag = HexAsciiConverter::convert(tag.first);
    if (!asciiTag.isEmpty() && asciiTag.length() == 7 && tag.second > bestCount) {
      bestCount = tag.second;
      bestTag = asciiTag;
    }
  }

  return bestTag;
}

void bootstrapNetworkIfNeeded() {
  if (!lanReady || networkBootstrapDone) {
    return;
  }

  for (int i = 0; i < 3; ++i) {
    http.safeLinkStatus();
    delay(100);
  }

  updater.performFirmwareUpdate(
      [](float progress) {
        Serial.println("update progress: " + String(progress * 100) + "%");
      },
      [](FirmwareUpdateResult result) {
        if (result == FirmwareUpdateResult::SUCCESS) {
          Serial.println("successed to update firmware!");
        } else if (result == FirmwareUpdateResult::NO_UPDATE_NEEDED) {
          Serial.println("firmware is up-to-date. (no update needed)");
        } else {
          Serial.println("failed to update firmware: " +
                         String(static_cast<int>(result)));
        }
      });

  for (int i = 0; i < 5; ++i) {
    http.safeLinkStatus();
    delay(100);
  }

  int session = http.setSession();
  int configuredLoadingTime = http.loadLoadingTime();

  if (session <= 0 || configuredLoadingTime <= 0) {
    Serial.println("session bootstrap failed; keep local sensing alive");
  } else {
    readingTimeUs = static_cast<uint64_t>(configuredLoadingTime) * 1000ULL * 1000ULL;
    Serial.printf("session=%d loadingTime=%d sec\n", session,
                  configuredLoadingTime);
  }

  networkBootstrapDone = true;
}

void attemptLanRecovery(bool forceLog) {
  if (networkAttemptCompleted) {
    return;
  }

  const unsigned long now = millis();
  if (!forceLog && (now - lastLanAttemptMs) < LAN_RECOVERY_INTERVAL_MS) {
    return;
  }

  lastLanAttemptMs = now;
  if (forceLog) {
    Serial.println("LAN connecting...");
  }

  if (lan.begin()) {
    lanReady = true;
    Serial.println("LAN connected");
    Serial.print("IP: ");
    Serial.println(lan.getLocalIP());
    return;
  }

  lanReady = false;
  networkAttemptCompleted = true;
  if (forceLog) {
    Serial.println("LAN unavailable. Local sensing stays active.");
  }
}

void startNetworkTask() {
  if (networkQueue == nullptr) {
    networkQueue = xQueueCreate(10, sizeof(PendingNetworkEvent));
  }

  xTaskCreate(
      [](void *param) {
        PendingNetworkEvent event;
        unsigned long lastLinkCheckMs = 0;

        while (1) {
          if (xQueueReceive(networkQueue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (lanReady) {
              switch (event.type) {
              case PendingNetworkEvent::FittingDoorOpened:
                http.openFittingRoomDoor();
                break;
              case PendingNetworkEvent::FittingDoorClosed:
                http.closeFittingRoomDoor();
                break;
              case PendingNetworkEvent::ShowroomDoorOpened:
                http.openShowroomDoor();
                break;
              case PendingNetworkEvent::ShowroomDoorClosed:
                http.closeShowroomDoor();
                break;
              case PendingNetworkEvent::SendBestTag: {
                String payload = String(event.payload);
                if (payload.length() > 0) {
                  http.sendTag(payload);
                }
                break;
              }
              }
            }
          }

          const unsigned long now = millis();
          if (lanReady && (now - lastLinkCheckMs) >= 5000UL) {
            lastLinkCheckMs = now;
            if (http.safeLinkStatus() == EthernetLinkStatus::LinkOFF) {
              lanReady = false;
              networkBootstrapDone = true;
              networkAttemptCompleted = true;
              Serial.println("LAN link lost. Local sensing stays active.");
            }
          }

          if (lanReady && !networkBootstrapDone) {
            bootstrapNetworkIfNeeded();
          }
        }
      },
      "NetworkTask", 8192, NULL, 1, NULL);
}

void startRfidFinalizeTask() {
  xTaskCreate(
      [](void *param) {
        while (1) {
          if (lastCloseTimeUs != 0) {
            const uint64_t now = esp_timer_get_time();
            const uint64_t elapsed = now - lastCloseTimeUs;

            if (elapsed >= readingTimeUs) {
              Serial.println("[RFID] Scan finished");
              reader->stopRead();
              led.blink(100, 100);

              std::map<String, int> tags = reader->getTags();
              for (const auto &tag : tags) {
                Serial.println("RFID summary " + tag.first + " count=" +
                               String(tag.second));
              }

              String bestTag = selectBestAsciiTag(tags);
              if (!bestTag.isEmpty()) {
                Serial.println("[RFID] Best ASCII tag: " + bestTag);
                enqueueNetworkEvent(PendingNetworkEvent::SendBestTag, bestTag);
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

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(200);
  Serial.println("System boot");

  HttpManageX::initMutex();

  reader->begin();
  reader->onRead([](String tag) {
    Serial.printf("RFID read %s\n", tag.c_str());
  });

  led.begin();
  led.blink(500);

  fittingroomDoor.begin({
      {
          .event = DoorEvent::Closed,
          .delayMs = NO_DELAY,
          .action = []() {
            Serial.println("[event]\tFitting room door closed");
            Serial.println("RFID scan started");
            led.blink(100, 100);
            reader->clearTags();
            reader->startRead();
            lastCloseTimeUs = esp_timer_get_time();
            enqueueNetworkEvent(PendingNetworkEvent::FittingDoorClosed);
          },
      },
      {
          .event = DoorEvent::Opened,
          .delayMs = NO_DELAY,
          .action = []() {
            Serial.println("[event]\tFitting room door opened");
            Serial.println("RFID scan cancelled");
            reader->stopRead();
            reader->clearTags();
            lastCloseTimeUs = 0;
            enqueueNetworkEvent(PendingNetworkEvent::FittingDoorOpened);
            led.on();
          },
      },
  });

  showroomDoor.begin({
      {
          .event = DoorEvent::Closed,
          .delayMs = NO_DELAY,
          .action = []() {
            Serial.println("Showroom door closed");
            enqueueNetworkEvent(PendingNetworkEvent::ShowroomDoorClosed);
            led.on();
          },
      },
      {
          .event = DoorEvent::Opened,
          .delayMs = NO_DELAY,
          .action = []() {
            Serial.println("Showroom door open");
            enqueueNetworkEvent(PendingNetworkEvent::ShowroomDoorOpened);
            led.on();
          },
      },
  });

  startNetworkTask();
  startRfidFinalizeTask();
  attemptLanRecovery(true);
  if (lanReady) {
    bootstrapNetworkIfNeeded();
  } else {
    networkBootstrapDone = true;
  }
  led.on();
}

void loop() {
  static bool lastPirState = false;
  bool currentPirState = (digitalRead(D0) == HIGH);

  if (currentPirState != lastPirState) {
    lastPirState = currentPirState;
    Serial.printf("PIR %s\n", currentPirState ? "motion" : "idle");
  }

  delay(50);
}
