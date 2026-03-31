#include <ArduinoJson.h>

#include "ButtonSensor.h"
#include "Config.h"
#include "DigitalOut.h"
#include "FirmwareUpdater.h"
#include "MqttService.h"
#include "TimeSync.h"
#include "UuidStore.h"
#include "WifiManager.h"

const char* OTA_URL = "http://gym907-0001.iptime.org:3004";
const char* PROJ_ID = "41742426-d035-4756-ba17-f542ea75ab02";
constexpr int FW_VERSION_CODE = 53;

WifiManager wifi(WIFI_SSID, WIFI_PASSWORD);
FirmwareUpdater updater(OTA_URL, PROJ_ID, FW_VERSION_CODE);
TimeSync dateTime;
MqttService mqtt;
UuidStore uuid;

ButtonSensor button(Config::kButtonPin, true);
DigitalOut relay(Config::kRelayPin);
DigitalOut led1(Config::kAuxLedPin, false);
DigitalOut led2(Config::kLedPin, false);

String controlTopic = "smart_plug/{uuid}";

void serviceDelay(unsigned long durationMs, bool serviceWifi = false) {
  unsigned long startedMs = millis();
  while (millis() - startedMs < durationMs) {
    led2.tick();
    if (serviceWifi) {
      wifi.tick(Config::kWifiRetryDelayMs);
    }
    delay(10);
  }
}

String statusPayload(bool isOn) {
  return "{\"state\":\"" + String(isOn ? "on" : "off") + "\"}";
}

void publishStatus() {
  mqtt.publish(controlTopic + "/status", statusPayload(relay.isOn()));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("--- Boot Start ---"));
  Serial.printf("FW Code: %d\n", FW_VERSION_CODE);

  button.begin();
  led1.begin();
  led2.begin();
  relay.begin();

  led2.blink(10);
  serviceDelay(Config::kBootBlinkMs);
  led2.stopBlink();
  led2.off();

  led2.blink(500);
  serviceDelay(2000);
  led2.stopBlink();
  wifi.begin();

  led2.blink(250);
  serviceDelay(2000, true);
  led2.stopBlink();
  updater.performFirmwareUpdate(
      [](float progress) { Serial.printf("Update Progress: %.2f%%\n", progress * 100.0f); },
      [](FirmwareUpdateResult result) { Serial.printf("Update Result: %d\n", static_cast<int>(result)); });

  led2.blink(100);
  serviceDelay(2000, true);
  led2.stopBlink();
  uuid.begin();

  controlTopic = "smart_plug/" + uuid.load();

  dateTime.begin();

  button.onPressed([]() {
    bool isOn = relay.isOn();
    if (isOn) {
      relay.off();
      led1.off();
    } else {
      relay.on();
      led1.on();
    }

    publishStatus();
  });

  mqtt.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
  mqtt.subscribe(controlTopic + "/command");

  mqtt.setCallback([](const String& topic, const String& payload) {
    Serial.printf("[MQTT] recv topic=%s payload=%s\n", topic.c_str(), payload.c_str());

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      Serial.printf("[MQTT] JSON parse error: %s\n", err.c_str());
      return;
    }

    if (!doc.containsKey("cmd")) {
      Serial.println(F("[MQTT] invalid payload: missing cmd"));
      return;
    }

    String cmd = doc["cmd"].as<String>();
    cmd.toLowerCase();

    Serial.printf("[MQTT] cmd=%s\n", cmd.c_str());

    if (cmd == "on") {
      relay.on();
      led1.on();
      Serial.println(F("[RELAY] ON"));
      mqtt.publish(controlTopic + "/status", statusPayload(true));
      return;
    }

    if (cmd == "off") {
      relay.off();
      led1.off();
      Serial.println(F("[RELAY] OFF"));
      mqtt.publish(controlTopic + "/status", statusPayload(false));
      return;
    }

    if (cmd == "status") {
      publishStatus();
      Serial.println(F("[MQTT] status sent"));
      return;
    }

    Serial.println(F("[MQTT] unknown cmd"));
  });
}

void loop() {
  static unsigned long lastPrint = 0;
  unsigned long now = millis();

  led1.tick();
  led2.tick();

  wifi.tick(Config::kWifiRetryDelayMs);
  mqtt.tick(Config::kMqttRetryDelayMs);
  dateTime.tick();
  button.tick(Config::kDebounceMs);

  if (!wifi.connected()) {
    led2.blink(500);
  } else if (!mqtt.connected()) {
    led2.blink(100);
  } else {
    led2.off();
  }

  if (now - lastPrint > 5000) {
    lastPrint = now;
    publishStatus();
  }

  delay(100);
}
