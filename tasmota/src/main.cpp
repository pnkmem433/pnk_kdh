#include <ArduinoJson.h>

#include "ButtonSensor.h"
#include "Config.h"
#include "DigitalOut.h"
#include "FirmwareUpdater.h"
#include "MqttService.h"
#include "TimeSync.h"
#include "UuidStore.h"
#include "WifiManager.h"

#define FW_VER_MAJOR 0
#define FW_VER_MINOR 5
#define FW_VER_PATCH 2

#define FW_VERSION_CODE ((FW_VER_MAJOR * 100) + (FW_VER_MINOR * 10) + FW_VER_PATCH)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define FW_VERSION_STRING STR(FW_VER_MAJOR) "." STR(FW_VER_MINOR) "." STR(FW_VER_PATCH)

__attribute__((used))
const char FW_VERSION[] = "SMARTPLUG_FW_VERSION:" FW_VERSION_STRING;

WifiManager wifi(WIFI_SSID, WIFI_PASSWORD);
FirmwareUpdater updater(OTA_MANIFEST_URL);

TimeSync dateTime;
MqttService mqtt;
UuidStore uuid;

ButtonSensor button(Config::kButtonPin, true);
DigitalOut relay(Config::kRelayPin);
DigitalOut blueLed(Config::kLedPin, false);
DigitalOut led2(Config::kAuxLedPin, false);

String controlTopic = "smart_plug/{uuid}";
String commandTopic = "smart_plug/{uuid}/command";
String statusTopic = "smart_plug/{uuid}/status";

unsigned long lastPublishMs = 0;
bool lastWifiState = false;

void serviceDelay(unsigned long durationMs, bool serviceWifi = false) {
  unsigned long startedMs = millis();
  while (millis() - startedMs < durationMs) {
    blueLed.tick();
    if (serviceWifi) {
      wifi.tick(Config::kWifiRetryDelayMs);
    }
    delay(10);
  }
}

String statusPayload(bool isOn) {
  StaticJsonDocument<128> doc;
  doc["state"] = isOn ? "on" : "off";
  doc["wifi"] = wifi.connected();
  doc["ip"] = wifi.connected() ? wifi.localIp().toString() : "";

  String payload;
  serializeJson(doc, payload);
  return payload;
}

void applyRelayState(bool newState) {
  if (newState) {
    relay.on();
  } else {
    relay.off();
  }

  mqtt.publish(statusTopic, statusPayload(newState), true);
}

void publishStatus() {
  mqtt.publish(statusTopic, statusPayload(relay.isOn()), true);
}

void updateTopics() {
  controlTopic = "smart_plug/" + uuid.load();
  commandTopic = controlTopic + "/command";
  statusTopic = controlTopic + "/status";
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("--- Boot Start ---"));
  Serial.printf("FW Info: %s\n", FW_VERSION);
  Serial.printf("FW Code: %d\n", FW_VERSION_CODE);

  button.begin();
  blueLed.begin();
  relay.begin();

  // 부팅 시그니처: 10ms 간격으로 3초 동안 점멸
  blueLed.blink(10);
  serviceDelay(Config::kBootBlinkMs);
  blueLed.stopBlink();
  blueLed.off();

  // WiFi 연결 단계
  blueLed.blink(500);
  serviceDelay(2000);
  blueLed.stopBlink();
  wifi.begin();

  // 펌웨어 업데이트 확인 단계
  blueLed.blink(250);
  serviceDelay(2000, true);
  blueLed.stopBlink();
  blueLed.on();
  updater.performFirmwareUpdate(
      [](float progress) { Serial.printf("Update Progress: %.2f%%\n", progress * 100.0f); },
      [](FirmwareUpdateResult result) {
        Serial.printf("Update Result: %d\n", static_cast<int>(result));
      });
  if (wifi.connected()) {
    Serial.printf("WiFi connected. IP: %s\n", wifi.localIp().toString().c_str());
  } else {
    Serial.println(F("WiFi not connected during OTA check"));
  }

  // UUID 및 MQTT 설정 단계
  blueLed.blink(100);
  serviceDelay(2000, true);
  blueLed.stopBlink();
  blueLed.off();
  uuid.begin();
  updateTopics();
  dateTime.begin();

  button.onPressed([]() {
    bool newState = !relay.isOn();
    applyRelayState(newState);
  });

  mqtt.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
  mqtt.subscribe(commandTopic);
  mqtt.setCallback([](const String& topic, const String& payload) {
    if (topic != commandTopic) {
      return;
    }

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
      return;
    }
    if (!doc.containsKey("cmd")) {
      return;
    }

    String cmd = doc["cmd"].as<String>();
    cmd.toLowerCase();

    if (cmd == "on" || cmd == "off") {
      applyRelayState(cmd == "on");
    } else if (cmd == "status") {
      publishStatus();
    }
  });
}

void loop() {
  unsigned long now = millis();

  blueLed.tick();

  wifi.tick(Config::kWifiRetryDelayMs);
  mqtt.tick(Config::kMqttRetryDelayMs);
  dateTime.tick();
  button.tick(Config::kDebounceMs);

  bool wifiConnected = wifi.connected();
  if (wifiConnected != lastWifiState) {
    lastWifiState = wifiConnected;
    Serial.printf("WiFi %s\n", wifiConnected ? "connected" : "disconnected");
    if (wifiConnected) {
      Serial.printf("IP: %s\n", wifi.localIp().toString().c_str());
    }
  }

  if (!wifi.connected()) {
    blueLed.blink(500);
  } else if (!mqtt.connected()) {
    blueLed.blink(100);
  } else {
    blueLed.on();
  }

  if (mqtt.connected() && now - lastPublishMs >= Config::kStatusPublishIntervalMs) {
    lastPublishMs = now;
    publishStatus();
  }

  delay(10);
}
