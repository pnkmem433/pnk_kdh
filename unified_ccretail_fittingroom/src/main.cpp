#include <map>
#include <vector>

#include <UIPEthernet.h>

#include "MqttLite.h"
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
#ifndef MQTT_HOST
#error "MQTT_HOST is not defined. Set -DMQTT_HOST=\\\"...\\\" in platformio.ini"
#endif
#ifndef MQTT_PORT
#error "MQTT_PORT is not defined. Set -DMQTT_PORT=<int> in platformio.ini"
#endif
#ifndef MQTT_USER
#error "MQTT_USER is not defined. Set -DMQTT_USER=\\\"...\\\" in platformio.ini"
#endif
#ifndef MQTT_PASS
#error "MQTT_PASS is not defined. Set -DMQTT_PASS=\\\"...\\\" in platformio.ini"
#endif
#ifndef ENABLE_MQTT
#define ENABLE_MQTT 1
#endif

#ifndef DOOR_SENSOR_PIN
#define DOOR_SENSOR_PIN D1
#endif
#ifndef DOOR_SENSOR_LABEL
#define DOOR_SENSOR_LABEL "D1"
#endif
#ifndef DOOR_SENSOR_CLOSED_LEVEL
#define DOOR_SENSOR_CLOSED_LEVEL HIGH
#endif
#ifndef DOOR_SENSOR_PIN_MODE
#define DOOR_SENSOR_PIN_MODE INPUT
#endif
#ifndef DOOR_DEBOUNCE_MS
#define DOOR_DEBOUNCE_MS 250
#endif
#ifndef DOOR_SCAN_WINDOW_MS
#define DOOR_SCAN_WINDOW_MS 10000UL
#endif
#ifndef LAN_CS_PIN
#define LAN_CS_PIN D5
#endif
#ifndef LAN_CS_LABEL
#define LAN_CS_LABEL "D5"
#endif
#ifndef RFID_RX_PIN
#define RFID_RX_PIN D7
#endif
#ifndef RFID_TX_PIN
#define RFID_TX_PIN D6
#endif
#ifndef RFID_RX_LABEL
#define RFID_RX_LABEL "D7"
#endif
#ifndef RFID_TX_LABEL
#define RFID_TX_LABEL "D6"
#endif
#ifndef RFID_BAUD_RATE
#define RFID_BAUD_RATE 115200
#endif
#ifndef LED_STRIP_UUID
#define LED_STRIP_UUID "9642C3BA-FC4A-4B07-A0E0-9153D323EC06"
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
#ifndef DOOR_PATCH_RETRY_DELAY_MS
#define DOOR_PATCH_RETRY_DELAY_MS 300UL
#endif

namespace {
constexpr unsigned long NET_RETRY_INTERVAL_MS = 5000;

enum class DoorPhase {
  Open,
  ClosedScanning,
  ClosedUsing,
};

enum class BootDoorState {
  Unknown,
  Open,
  Closed,
};

enum class LocalLedState {
  Off,
  On,
  Blink100,
  Blink250,
  Blink500,
};

enum class NetworkActionType {
  SyncDoorOpen,
  SyncDoorClosed,
  SendSessionItems,
};

struct NetworkAction {
  NetworkActionType type;
};

LAN lan(LAN_CS_PIN);
LED led = LED(D4);
RfidReader *reader =
    new FonkanFF704RfidReader(Serial1, RFID_RX_PIN, RFID_TX_PIN, RFID_BAUD_RATE);

Mqtt mqtt = Mqtt({
    .host = MQTT_HOST,
    .port = MQTT_PORT,
    .user = MQTT_USER,
    .password = MQTT_PASS,
    .useTls = false,
    .caCert = "",
    .autoReconnect = true,
});

volatile bool ethernetReady = false;
volatile bool mqttReady = false;
bool rfidReading = false;
volatile bool networkBusy = false;
unsigned long lastLanRetryMs = 0;
unsigned long lastMqttRetryMs = 0;
DoorPhase doorPhase = DoorPhase::Open;
BootDoorState bootDoorState = BootDoorState::Unknown;
unsigned long closedStartedMs = 0;
bool bootStateInitialized = false;
bool lastSentDoorClosedKnown = false;
bool lastSentDoorClosed = false;
unsigned long doorOpenEventCount = 0;
unsigned long doorClosedEventCount = 0;
unsigned long apiEventSequence = 0;
String desiredLedMode = "READY";
unsigned long localLedErrorUntilMs = 0;
LocalLedState appliedLocalLedState = LocalLedState::Off;
Sensor doorSensor = Sensor(DOOR_SENSOR_PIN, DOOR_SENSOR_PIN_MODE);
QueueHandle_t networkActionQueue = nullptr;

String csvSafeField(const String &value) {
  String out = value;
  out.replace(",", ";");
  out.replace("\r", " ");
  out.replace("\n", " ");
  out.replace("\"", "'");
  return out;
}

void emitApiCsv(const char *eventName, int statusCode, const char *result,
                unsigned long deviceMs, int attemptCount,
                const String &detail) {
  const unsigned long eventId = ++apiEventSequence;
  Serial.printf("###AI_CSV###,%lu,%s,%d,%s,%lu,%d,%s\n", eventId, eventName,
                statusCode, result, deviceMs, attemptCount,
                csvSafeField(detail).c_str());
}

String ledTopicMode() {
  return String("fittingroom/led_strip/") + String(LED_STRIP_UUID) +
         "/command/mode";
}

bool readDoorClosed() {
  return digitalRead(DOOR_SENSOR_PIN) == DOOR_SENSOR_CLOSED_LEVEL;
}

Event closedSensorEvent() {
  return DOOR_SENSOR_CLOSED_LEVEL == HIGH ? Event::High : Event::Low;
}

Event openSensorEvent() {
  return DOOR_SENSOR_CLOSED_LEVEL == HIGH ? Event::Low : Event::High;
}

void applyLocalLedState() {
  LocalLedState nextState = LocalLedState::Off;

  if (localLedErrorUntilMs != 0 &&
      static_cast<long>(millis() - localLedErrorUntilMs) < 0) {
    nextState = LocalLedState::Blink500;
  } else if (!ethernetReady) {
    nextState = LocalLedState::Blink250;
  } else if (doorPhase == DoorPhase::ClosedScanning || rfidReading) {
    nextState = LocalLedState::Blink100;
  } else if (doorPhase == DoorPhase::Open) {
    nextState = LocalLedState::Off;
  } else {
    nextState = LocalLedState::On;
  }

  if (nextState == appliedLocalLedState) {
    return;
  }

  appliedLocalLedState = nextState;

  switch (nextState) {
  case LocalLedState::Off:
    led.off();
    break;
  case LocalLedState::On:
    led.on();
    break;
  case LocalLedState::Blink100:
    led.blink(100, 100);
    break;
  case LocalLedState::Blink250:
    led.blink(250, 250);
    break;
  case LocalLedState::Blink500:
    led.blink(500, 500);
    break;
  }
}

void triggerLocalLedErrorBlink() {
  localLedErrorUntilMs = millis() + 2000UL;
  applyLocalLedState();
}

bool ensureLanForHttp() {
  if (lan.isConnected()) {
    ethernetReady = true;
    return true;
  }

  ethernetReady = lan.begin();
  if (!ethernetReady) {
    Serial.println("HTTP ?꾩넚 ?앸왂: LAN ?곌껐???대젮媛 ?덉뒿?덈떎.");
  }
  applyLocalLedState();
  return ethernetReady;
}

DoorApiResult executeDoorRequest(
    const std::function<DoorApiResult(DoorApi &)> &request) {
  DoorApiResult result = {
      .statusCode = -1,
      .body = "",
      .alreadyMatched = false,
  };

  bool requestRan = lan.withEthernet([&](EthernetClient &client) {
    Http scopedHttp(&client);
    DoorApi scopedDoorApi(&scopedHttp, API_HOST, API_PORT);

    scopedHttp.begin();
    scopedDoorApi.begin();
    scopedDoorApi.setFittingRoomId(FITTING_ROOM_ID);
    result = request(scopedDoorApi);
    return true;
  });

  if (!requestRan) {
    Serial.println("HTTP ?꾩넚 ?ㅽ뙣: Ethernet mutex瑜??띾뱷?섏? 紐삵뻽?듬땲??");
  }

  return result;
}

void ensureMqttConnected() {
  if (!ENABLE_MQTT) {
    return;
  }

  if (!ethernetReady || mqttReady || networkBusy) {
    return;
  }
  mqtt.connect();
}

void publishLedMode(const char *mode) {
  desiredLedMode = String(mode);

  if (!ENABLE_MQTT) {
    return;
  }

  if (!mqttReady) {
    return;
  }

  mqtt.publish({
      .topic = ledTopicMode(),
      .payload = String(mode),
      .qos = 0,
      .retain = false,
      .retry = 0,
      .timeoutMs = 2000,
      .onAck = nullptr,
      .onFail = [mode]() {},
  });
}

void stopRfidAndClearTags() {
  if (rfidReading) {
    reader->stopRead();
    rfidReading = false;
  }
  reader->clearTags();
}

void startRfidWindow() {
  reader->clearTags();
  if (!rfidReading) {
    reader->startRead();
    rfidReading = true;
  }
}

std::vector<String> collectQualifiedSkus() {
  std::vector<String> skus;
  std::map<String, int> tags = reader->getTags();

  for (const auto &entry : tags) {
    const String &tag = entry.first;
    const int count = entry.second;

    if (count < RFID_MIN_READ_COUNT) {
      continue;
    }

    if (tag.length() < RFID_TAG_SUBSTRING_END) {
      Serial.printf("RFID ?쒓렇 湲몄씠 遺議깆쑝濡??쒖쇅: %s (%d??\n",
                    tag.c_str(), count);
      continue;
    }

    String sku = tag.substring(RFID_TAG_SUBSTRING_START, RFID_TAG_SUBSTRING_END);
    if (sku.length() == 0) {
      continue;
    }

    bool exists = false;
    for (const String &existing : skus) {
      if (existing == sku) {
        exists = true;
        break;
      }
    }

    if (!exists) {
      Serial.printf("RFID ?꾩넚 ?꾨낫 SKU: %s (?먮낯 ?쒓렇 %s, %d??\n",
                    sku.c_str(), tag.c_str(), count);
      skus.push_back(sku);
    }
  }

  return skus;
}

void patchDoorStatus(bool isClosed) {
  networkBusy = true;
  const unsigned long requestStartedMs = millis();
  int attemptCount = 0;
  if (!ensureLanForHttp()) {
    triggerLocalLedErrorBlink();
    emitApiCsv(isClosed ? "door_closed" : "door_open", -2, "LAN_UNAVAILABLE",
               requestStartedMs, attemptCount,
               String("open_count=") + doorOpenEventCount + ";closed_count=" +
                   doorClosedEventCount);
    networkBusy = false;
    return;
  }

  Serial.printf("HTTP request start: send door %s state to server.\n",
                isClosed ? "closed" : "open");

  ++attemptCount;
  DoorApiResult result = executeDoorRequest(
      [isClosed](DoorApi &api) { return isClosed ? api.closeDoor() : api.openDoor(); });
  if (result.statusCode < 0) {
    Serial.printf("[HTTP][retry] first attempt failed for door %s, retrying after %lu ms\n",
                  isClosed ? "closed" : "open", DOOR_PATCH_RETRY_DELAY_MS);
    delay(DOOR_PATCH_RETRY_DELAY_MS);
    ++attemptCount;
    result = executeDoorRequest(
        [isClosed](DoorApi &api) { return isClosed ? api.closeDoor() : api.openDoor(); });
  }

  if (result.alreadyMatched) {
    Serial.printf("Server already matches door %s state. Keeping current state.\n",
                  isClosed ? "closed" : "open");
  } else if (result.statusCode < 200 || result.statusCode >= 300) {
    Serial.printf("Door %s send failed or response parse failed. status=%d\n",
                  isClosed ? "closed" : "open", result.statusCode);
    triggerLocalLedErrorBlink();
  } else {
    Serial.printf("Door %s state sent successfully. status=%d\n",
                  isClosed ? "closed" : "open", result.statusCode);
  }

  emitApiCsv(
      isClosed ? "door_closed" : "door_open", result.statusCode,
      result.alreadyMatched
          ? "ALREADY_MATCHED"
          : ((result.statusCode >= 200 && result.statusCode < 300) ? "OK"
                                                                   : "FAILED"),
      requestStartedMs, attemptCount,
      String("open_count=") + doorOpenEventCount + ";closed_count=" +
          doorClosedEventCount + ";already_matched=" +
          (result.alreadyMatched ? "1" : "0"));
  networkBusy = false;
}

void sendSessionItemsNow() {
  std::vector<String> skus = collectQualifiedSkus();
  const unsigned long requestStartedMs = millis();
  if (skus.empty()) {
    Serial.println("RFID send skipped: no qualified items.");
    emitApiCsv("rfid_session_items", 0, "SKIPPED_EMPTY", requestStartedMs, 0,
               "sku_count=0");
    return;
  }

  networkBusy = true;
  if (!ensureLanForHttp()) {
    triggerLocalLedErrorBlink();
    emitApiCsv("rfid_session_items", -2, "LAN_UNAVAILABLE", requestStartedMs, 0,
               String("sku_count=") + skus.size());
    networkBusy = false;
    return;
  }
  const int attemptCount = 1;

  DoorApiResult result =
      executeDoorRequest([&skus](DoorApi &api) { return api.sendTags(skus); });
  emitApiCsv("rfid_session_items", result.statusCode,
             (result.statusCode >= 200 && result.statusCode < 300) ? "OK"
                                                                    : "FAILED",
             requestStartedMs, attemptCount,
             String("sku_count=") + skus.size());
  if (result.statusCode >= 200 && result.statusCode < 300) {
    Serial.println("RFID items sent successfully.");
  } else {
    Serial.printf("RFID items send failed or response parse failed. status=%d\n",
                  result.statusCode);
    triggerLocalLedErrorBlink();
  }
  networkBusy = false;
  applyLocalLedState();
}

void enqueueNetworkAction(NetworkActionType type) {
  if (networkActionQueue == nullptr) {
    return;
  }

  NetworkAction action = {.type = type};
  if (xQueueSend(networkActionQueue, &action, 0) != pdTRUE) {
    Serial.println("Network action queue full. Dropping request.");
  }
}

void handleDoorOpen();
void handleDoorClosed();

void logDoorEventCounts(const char *stateLabel, unsigned long currentCount) {
  Serial.println();
  Serial.println("========== DOOR EVENT COUNT ==========");
  Serial.printf("[DOOR][COUNT] state=%s\n", stateLabel);
  Serial.printf("[DOOR][COUNT] current=%lu\n", currentCount);
  Serial.printf("[DOOR][COUNT] open=%lu\n", doorOpenEventCount);
  Serial.printf("[DOOR][COUNT] closed=%lu\n", doorClosedEventCount);
  Serial.printf("[DOOR][COUNT] total=%lu\n",
                doorOpenEventCount + doorClosedEventCount);
  Serial.println("======================================");
}

void processDoorOpenEvent() {
  if (!(lastSentDoorClosedKnown && !lastSentDoorClosed)) {
    ++doorOpenEventCount;
    logDoorEventCounts("OPEN", doorOpenEventCount);
  }
  handleDoorOpen();
}

void processDoorClosedEvent() {
  if (!(lastSentDoorClosedKnown && lastSentDoorClosed)) {
    ++doorClosedEventCount;
    logDoorEventCounts("CLOSED", doorClosedEventCount);
  }
  handleDoorClosed();
}

void handleDoorOpen() {
  if (lastSentDoorClosedKnown && !lastSentDoorClosed) {
    Serial.println("Door open ignored: already sent 0(open) and no new change.");
    return;
  }

  Serial.printf("Door open detected: value=0, pin=%s\n", DOOR_SENSOR_LABEL);
  Serial.println("Queued: READY LED and door-open PATCH request.");

  stopRfidAndClearTags();
  publishLedMode("READY");
  doorPhase = DoorPhase::Open;
  lastSentDoorClosedKnown = true;
  lastSentDoorClosed = false;
  applyLocalLedState();
  enqueueNetworkAction(NetworkActionType::SyncDoorOpen);

  Serial.println("Door open queued. Next 1 value will send door-closed.");
}

void handleDoorClosed() {
  if (lastSentDoorClosedKnown && lastSentDoorClosed) {
    Serial.println("Door closed ignored: already sent 1(closed) and no new change.");
    return;
  }

  Serial.printf("Door closed detected: value=1, pin=%s\n", DOOR_SENSOR_LABEL);
  Serial.printf("Queued: SCANNING LED, door-closed PATCH, RFID scan for %lu ms\n",
                DOOR_SCAN_WINDOW_MS);

  startRfidWindow();
  closedStartedMs = millis();
  publishLedMode("SCANNING");
  doorPhase = DoorPhase::ClosedScanning;
  lastSentDoorClosedKnown = true;
  lastSentDoorClosed = true;
  applyLocalLedState();
  enqueueNetworkAction(NetworkActionType::SyncDoorClosed);

  Serial.println("Door closed queued. PATCH request added to queue.");
}

void finishScanWindowIfNeeded() {
  if (doorPhase != DoorPhase::ClosedScanning) {
    return;
  }

  if (millis() - closedStartedMs < DOOR_SCAN_WINDOW_MS) {
    return;
  }

  if (rfidReading) {
    reader->stopRead();
    rfidReading = false;
  }

  std::map<String, int> tags = reader->getTags();
  Serial.printf("RFID scan complete: %u unique tags detected\n",
                static_cast<unsigned>(tags.size()));
  for (const auto &entry : tags) {
    Serial.printf("RFID tag %s read_count=%d\n",
                  entry.first.c_str(), entry.second);
  }

  enqueueNetworkAction(NetworkActionType::SendSessionItems);
  publishLedMode("USING");
  doorPhase = DoorPhase::ClosedUsing;
  applyLocalLedState();
  Serial.println("RFID scan finished. Next 0 value will send door-open.");
}

void startNetworkMonitorTask() {
  xTaskCreate(
      [](void *param) {
        bool lastLanState = ethernetReady;
        bool lastMqttState = mqttReady;

        while (1) {
          unsigned long now = millis();

          if (networkBusy) {
            applyLocalLedState();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
          }

          if ((!ethernetReady || !lan.isConnected()) &&
              now - lastLanRetryMs >= NET_RETRY_INTERVAL_MS) {
            lastLanRetryMs = now;
            ethernetReady = lan.begin();
            if (!ethernetReady) {
              mqttReady = false;
            }
          }

          if (ENABLE_MQTT && ethernetReady && !mqttReady &&
              now - lastMqttRetryMs >= NET_RETRY_INTERVAL_MS) {
            lastMqttRetryMs = now;
            ensureMqttConnected();
          }

          if (ethernetReady != lastLanState) {
            lastLanState = ethernetReady;
            if (ethernetReady) {
              Serial.printf("[LAN] connected: %s\n",
                            lan.getLocalIP().toString().c_str());
            } else {
              Serial.println("[LAN] disconnected");
            }
            applyLocalLedState();
          }

          if (ENABLE_MQTT && mqttReady != lastMqttState) {
            lastMqttState = mqttReady;
            if (!mqttReady) {
              Serial.println("[MQTT] disconnected");
            }
          }

          vTaskDelay(pdMS_TO_TICKS(500));
        }
      },
      "NetworkMonitorTask",
      4096,
      NULL,
      1,
      NULL);
}

void initDoorSensor() {
  if (!bootStateInitialized) {
    bootStateInitialized = true;
    if (readDoorClosed()) {
      bootDoorState = BootDoorState::Closed;
      doorPhase = DoorPhase::ClosedUsing;
      Serial.println("Boot state: value=1, door closed");
      publishLedMode("USING");
      applyLocalLedState();
      Serial.println("Boot sync: align server to current door-closed state.");
      patchDoorStatus(true);
      lastSentDoorClosedKnown = true;
      lastSentDoorClosed = true;
      Serial.println("Next 0 value will send door-open to server.");
    } else {
      bootDoorState = BootDoorState::Open;
      doorPhase = DoorPhase::Open;
      Serial.println("Boot state: value=0, door open");
      publishLedMode("READY");
      applyLocalLedState();
      Serial.println("Boot sync: align server to current door-open state.");
      patchDoorStatus(false);
      lastSentDoorClosedKnown = true;
      lastSentDoorClosed = false;
      Serial.println("Next 1 value will send door-closed to server.");
    }
  }

  doorSensor.begin({
      {.event = openSensorEvent(),
       .delayMs = DOOR_DEBOUNCE_MS,
       .action = []() { processDoorOpenEvent(); }},
      {.event = closedSensorEvent(),
       .delayMs = DOOR_DEBOUNCE_MS,
       .action = []() { processDoorClosedEvent(); }},
  });

  xTaskCreate(
      [](void *param) {
        while (1) {
          applyLocalLedState();
          finishScanWindowIfNeeded();
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      },
      "DoorPhaseTask",
      4096,
      NULL,
      1,
      NULL);
}

void startNetworkActionTask() {
  if (networkActionQueue == nullptr) {
    networkActionQueue = xQueueCreate(10, sizeof(NetworkAction));
  }

  xTaskCreate(
      [](void *param) {
        NetworkAction action;
        while (1) {
          if (xQueueReceive(networkActionQueue, &action, portMAX_DELAY) ==
              pdTRUE) {
            switch (action.type) {
            case NetworkActionType::SyncDoorOpen:
              patchDoorStatus(false);
              break;
            case NetworkActionType::SyncDoorClosed:
              patchDoorStatus(true);
              break;
            case NetworkActionType::SendSessionItems:
              sendSessionItemsNow();
              break;
            }
          }
        }
      },
      "NetworkActionTask",
      6144,
      NULL,
      1,
      NULL);
}
} // namespace

void setup() {
  Serial.begin(115200);
  delay(3000);

  led.begin();
  applyLocalLedState();

  if (ENABLE_MQTT) {
    mqtt.onConnect([](bool isReconnect) {
      mqttReady = true;
      publishLedMode(desiredLedMode.c_str());
    });
    mqtt.onDisconnect([]() { mqttReady = false; });
  }

  reader->onRead([](String tag) {
    Serial.printf("Read tag: %s\n", tag.c_str());
  });
  reader->begin();

  if (ENABLE_MQTT) {
    mqtt.begin();
  }
  startNetworkActionTask();

  Serial.println("[LAN] connecting...");
  ethernetReady = lan.begin();
  applyLocalLedState();
  if (ethernetReady) {
    Serial.printf("[LAN] connected: %s\n", lan.getLocalIP().toString().c_str());
    if (ENABLE_MQTT) {
      ensureMqttConnected();
    }
  } else {
    Serial.println("[LAN] not connected");
  }

  startNetworkMonitorTask();
  initDoorSensor();
}

void loop() { delay(10); }

