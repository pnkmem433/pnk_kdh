#include <Arduino.h>
#include <ArduinoJson.h>

#include "AppConfig.h"
#include "BoardConfig.h"
#include "DigitalOut.h"
#include "FirmwareUpdater.h"
#include "dateTimeManagerX.h"
#include "digital_sensor.h"
#include "mqtt.h"
#include "uuid.h"
#include "wifiManager.h"

__attribute__((used))
const char FW_VERSION[] = "SMARTPLUG_FW_VERSION:" FW_VERSION_STRING;

WifiManager wifi({.ssid = WIFI_SSID, .password = WIFI_PASSWORD});
FirmwareUpdater updater =
    FirmwareUpdater(OTA_URL, OTA_PROJECT_ID, FW_VERSION_CODE, CHIP_TYPE, CURRENT_FIRMWARE_FAMILY);
DateTimeManagerX dateTime = DateTimeManagerX();
Mqtt mqtt = Mqtt();
Uuid uuid = Uuid();

Sensor button = Sensor(BUTTON_PIN);
DigitalOut relay = DigitalOut(RELAY_PIN);
DigitalOut relayLed = DigitalOut(RELAY_LED_PIN, !RELAY_LED_ACTIVE_LOW);
DigitalOut statusLed = DigitalOut(STATUS_LED_PIN, !STATUS_LED_ACTIVE_LOW);

String controlTopic = "smart_plug/{uuid}";
constexpr unsigned long STATUS_PUBLISH_INTERVAL_MS = 1000UL;
unsigned long lastPublishMs = 0;

String statusPayload(bool isOn) {
  return "{\"state\":\"" + String(isOn ? "on" : "off") + "\"}";
}

void syncRelayLed() {
  if (relay.isOn()) {
    relayLed.on();
  } else {
    relayLed.off();
  }
}

void publishStatus() {
  if (mqtt.connected()) {
    mqtt.publish(controlTopic + "/status", statusPayload(relay.isOn()));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- Boot Start ---");
  Serial.printf("FW Info: %s\n", FW_VERSION);
  Serial.printf("FW Code: %d\n", FW_VERSION_CODE);

  button.begin();
  relay.begin();
  relayLed.begin();
  statusLed.begin();

  statusLed.blink(50);
  delay(1000);

  statusLed.blink(500);
  delay(2000);
  wifi.begin();

  statusLed.blink(250);
  delay(2000);
  updater.performFirmwareUpdate(
      [](float progress) { Serial.printf("Update Progress: %.2f%%\n", progress * 100.0f); },
      [](FirmwareUpdateResult result) { Serial.printf("Update Result: %d\n", (int)result); });

  statusLed.blink(100);
  delay(2000);
  uuid.begin();
  controlTopic = "smart_plug/" + uuid.load();
  dateTime.begin();

  button.listen({{
      .event = LOW,
      .delayMs = 0,
      .action = []() {
        if (relay.isOn()) {
          relay.off();
        } else {
          relay.on();
        }
        syncRelayLed();
        publishStatus();
      },
  }});

  mqtt.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
  mqtt.subscribe((controlTopic + "/command").c_str());

  mqtt.onReceived([](String topic, String payload) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
      return;
    }
    if (!doc.containsKey("cmd")) {
      return;
    }

    String cmd = doc["cmd"].as<String>();
    cmd.toLowerCase();

    if (cmd == "on") {
      relay.on();
      syncRelayLed();
      publishStatus();
    } else if (cmd == "off") {
      relay.off();
      syncRelayLed();
      publishStatus();
    } else if (cmd == "status") {
      publishStatus();
    }
  });

  syncRelayLed();
  publishStatus();
}

void loop() {
  const unsigned long now = millis();

  wifi.loop();
  mqtt.loop();
  dateTime.update();
  if (!wifi.isConnected()) {
    statusLed.blink(500);
  } else if (!mqtt.connected()) {
    statusLed.blink(50);
  } else {
    statusLed.off();
  }

  if (now - lastPublishMs >= STATUS_PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;
    publishStatus();
  }

  delay(10);
}
