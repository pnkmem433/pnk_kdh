#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#include "AppConfig.h"
#include "BoardConfig.h"
#include "DigitalOut.h"
#include "FirmwareUpdater.h"
#include "dateTimeManagerX.h"
#include "digital_sensor.h"
#include "mqtt.h"
#include "uuid.h"
#include "SmartPlugWifiManager.h"

__attribute__((used))
const char FW_VERSION[] = "SMARTPLUG_FW_VERSION:" FW_VERSION_STRING;

String mqttFirmwareVersion() {
  return "v" + String(FW_VERSION_CODE) + "_" + String(CHIP_TYPE) + "_" +
         String(CURRENT_FIRMWARE_FAMILY);
}

constexpr int OTA_MARKER_EEPROM_SIZE = 64;
constexpr int OTA_MARKER_ADDR = 16;
constexpr int OTA_MARKER_LENGTH = 16;
constexpr int BOOT_STATE_MAGIC_ADDR = 0;
constexpr int BOOT_STATE_VERSION_ADDR = 1;
constexpr int BOOT_STATE_FAMILY_ADDR = 5;
constexpr uint8_t BOOT_STATE_MAGIC = 0x42;

String readOtaMarker() {
  char buffer[OTA_MARKER_LENGTH + 1];
  for (int i = 0; i < OTA_MARKER_LENGTH; ++i) {
    buffer[i] = static_cast<char>(EEPROM.read(OTA_MARKER_ADDR + i));
  }
  buffer[OTA_MARKER_LENGTH] = '\0';
  return String(buffer);
}

void clearOtaMarker() {
  for (int i = 0; i < OTA_MARKER_LENGTH; ++i) {
    EEPROM.write(OTA_MARKER_ADDR + i, 0);
  }
  EEPROM.commit();
}

uint8_t currentFamilyCode() {
  const String family = String(CURRENT_FIRMWARE_FAMILY);
  return family.equalsIgnoreCase("tasmota") ? 2 : 1;
}

bool readBootState(uint32_t& versionCode, uint8_t& familyCode) {
  if (EEPROM.read(BOOT_STATE_MAGIC_ADDR) != BOOT_STATE_MAGIC) {
    versionCode = 0;
    familyCode = 0;
    return false;
  }

  versionCode = 0;
  for (int i = 0; i < 4; ++i) {
    versionCode |= static_cast<uint32_t>(EEPROM.read(BOOT_STATE_VERSION_ADDR + i)) << (8 * i);
  }
  familyCode = EEPROM.read(BOOT_STATE_FAMILY_ADDR);
  return true;
}

void writeBootState(uint32_t versionCode, uint8_t familyCode) {
  EEPROM.write(BOOT_STATE_MAGIC_ADDR, BOOT_STATE_MAGIC);
  for (int i = 0; i < 4; ++i) {
    EEPROM.write(
        BOOT_STATE_VERSION_ADDR + i,
        static_cast<uint8_t>((versionCode >> (8 * i)) & 0xFF));
  }
  EEPROM.write(BOOT_STATE_FAMILY_ADDR, familyCode);
  EEPROM.commit();
}

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
constexpr uint8_t WIFI_CONFIG_TRIGGER_COUNT = 6;
constexpr unsigned long WIFI_CONFIG_TRIGGER_WINDOW_MS = 4000UL;
unsigned long lastPublishMs = 0;
unsigned long wifiConfigSequenceStartMs = 0;
uint8_t wifiConfigPressCount = 0;
bool wifiConfigRequested = false;

String statusPayload(bool isOn) {
  StaticJsonDocument<96> doc;
  doc["state"] = isOn ? "on" : "off";
  doc["version"] = mqttFirmwareVersion();
  String payload;
  serializeJson(doc, payload);
  return payload;
}

String wifiInfoPayload() {
  String payload = "{\"configuredSsid\":\"" + String(WIFI_SSID) + "\"";
  payload += ",\"connected\":" + String(wifi.isConnected() ? "true" : "false");
  if (wifi.isConnected()) {
    payload += ",\"currentSsid\":\"" + WiFi.SSID() + "\"";
    payload += ",\"localIp\":\"" + WiFi.localIP().toString() + "\"";
  }
  payload += "}";
  return payload;
}

void syncRelayLed() {
  if (relay.isOn()) {
    relayLed.on();
  } else {
    relayLed.off();
  }
}

void registerWifiConfigPress() {
  const unsigned long now = millis();
  if (wifiConfigPressCount == 0 || (now - wifiConfigSequenceStartMs) > WIFI_CONFIG_TRIGGER_WINDOW_MS) {
    wifiConfigSequenceStartMs = now;
    wifiConfigPressCount = 1;
  } else {
    wifiConfigPressCount++;
  }

  if (wifiConfigPressCount >= WIFI_CONFIG_TRIGGER_COUNT) {
    wifiConfigRequested = true;
    wifiConfigPressCount = 0;
    wifiConfigSequenceStartMs = 0;
  }
}

void publishStatus() {
  if (mqtt.connected()) {
    mqtt.publish(controlTopic + "/status", statusPayload(relay.isOn()));
  }
}

void publishWifiInfo() {
  if (mqtt.connected()) {
    mqtt.publish(controlTopic + "/wifi", wifiInfoPayload());
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- Boot Start ---");
  Serial.printf("FW Info: %s\n", FW_VERSION);
  Serial.printf("FW Code: %d\n", FW_VERSION_CODE);

  EEPROM.begin(OTA_MARKER_EEPROM_SIZE);
  const String otaMarker = readOtaMarker();
  uint32_t previousVersionCode = 0;
  uint8_t previousFamilyCode = 0;
  const bool hasBootState = readBootState(previousVersionCode, previousFamilyCode);
  const uint8_t familyCode = currentFamilyCode();
  const bool firmwareChanged =
      !hasBootState ||
      previousVersionCode != static_cast<uint32_t>(FW_VERSION_CODE) ||
      previousFamilyCode != familyCode;
  const String bootSourceHint = otaMarker.length()
      ? "custom_ota"
      : (firmwareChanged ? "external_install" : "stable");
  clearOtaMarker();
  writeBootState(static_cast<uint32_t>(FW_VERSION_CODE), familyCode);

  button.begin();
  relay.begin();
  relayLed.begin();
  statusLed.begin();

  statusLed.blink(50);
  delay(1000);

  statusLed.blink(1000);
  delay(2000);
  wifi.begin(nullptr, false, [&]() { statusLed.blink(1000); });

  statusLed.blink(250);
  delay(2000);
  updater.setBootSourceHint(bootSourceHint);
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
  publishWifiInfo();
}

void loop() {
  const unsigned long now = millis();

  wifi.loop();
  mqtt.loop();
  dateTime.update();
  if (button.hasBeenLow()) {
    button.resetLowFlag();
    registerWifiConfigPress();
  }

  if (wifiConfigPressCount > 0 && (now - wifiConfigSequenceStartMs) > WIFI_CONFIG_TRIGGER_WINDOW_MS) {
    wifiConfigPressCount = 0;
    wifiConfigSequenceStartMs = 0;
  }

  if (wifiConfigRequested) {
    wifiConfigRequested = false;
    Serial.println("WiFi setup requested by 6 button presses");
    wifi.resetSettings();
    statusLed.blink(1000);
    const bool configured = wifi.openConfigPortal(nullptr, [&]() { statusLed.blink(1000); });
    if (configured) {
      publishWifiInfo();
      publishStatus();
    }
  }

  if (!wifi.isConnected()) {
    statusLed.blink(1000);
  } else if (!mqtt.connected()) {
    statusLed.blink(50);
  } else {
    statusLed.off();
  }

  if (now - lastPublishMs >= STATUS_PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;
    publishStatus();
    publishWifiInfo();
  }

  delay(10);
}
