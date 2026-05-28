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

constexpr size_t DEBUG_QUEUE_SIZE = 128;
constexpr int OTA_MARKER_EEPROM_SIZE = 64;
constexpr int OTA_MARKER_ADDR = 16;
constexpr int OTA_MARKER_LENGTH = 16;
constexpr int BOOT_STATE_MAGIC_ADDR = 0;
constexpr int BOOT_STATE_VERSION_ADDR = 1;
constexpr int BOOT_STATE_FAMILY_ADDR = 5;
constexpr uint8_t BOOT_STATE_MAGIC = 0x42;
constexpr uint8_t WIFI_CONFIG_TRIGGER_COUNT = 6;
constexpr unsigned long WIFI_CONFIG_TRIGGER_WINDOW_MS = 4000UL;

WifiManager wifi({.ssid = WIFI_SSID, .password = WIFI_PASSWORD});
FirmwareUpdater updater =
    FirmwareUpdater(OTA_URL, OTA_PROJECT_ID, FW_VERSION_CODE, CHIP_TYPE, CURRENT_FIRMWARE_FAMILY);
DateTimeManagerX dateTime = DateTimeManagerX();
Mqtt mqtt = Mqtt();
Uuid uuid = Uuid();

Sensor button = Sensor(BUTTON_PIN);
DigitalOut relay = DigitalOut(RELAY_PIN);
DigitalOut blueLed = DigitalOut(BLUE_LED_PIN, !BLUE_LED_ACTIVE_LOW);

String controlTopic = "smart_plug/{uuid}";
String debugTopic = "smart_plug/{uuid}/debug";
constexpr unsigned long STATUS_PUBLISH_INTERVAL_MS = 1000UL;
constexpr unsigned long DEBUG_FLUSH_INTERVAL_MS = 200UL;
unsigned long lastPublishMs = 0;
unsigned long lastDebugFlushMs = 0;
bool previousWifiConnected = false;
bool previousMqttConnected = false;
bool wifiConfigRequested = false;
unsigned long wifiConfigSequenceStartMs = 0;
uint8_t wifiConfigPressCount = 0;
String debugQueue[DEBUG_QUEUE_SIZE];
size_t debugQueueHead = 0;
size_t debugQueueCount = 0;

void syncRelayLed();

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

void pushDebug(const String& stage, const String& message) {
  StaticJsonDocument<192> doc;
  doc["ms"] = millis();
  doc["stage"] = stage;
  doc["msg"] = message;

  String payload;
  serializeJson(doc, payload);
  
  const size_t index = (debugQueueHead + debugQueueCount) % DEBUG_QUEUE_SIZE;
  debugQueue[index] = payload;
  if (debugQueueCount < DEBUG_QUEUE_SIZE) {
    debugQueueCount++;
  } else {
    debugQueueHead = (debugQueueHead + 1) % DEBUG_QUEUE_SIZE;
  }
}

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

void flushOneDebug() {
  if (!mqtt.connected() || debugQueueCount == 0 || debugTopic.indexOf("{uuid}") != -1) {
    return;
  }

  if (mqtt.publish(debugTopic, debugQueue[debugQueueHead])) {
    debugQueueHead = (debugQueueHead + 1) % DEBUG_QUEUE_SIZE;
    debugQueueCount--;
  }
}

String statusPayload(bool isOn) {
  StaticJsonDocument<96> doc;
  doc["state"] = isOn ? "on" : "off";
  doc["version"] = mqttFirmwareVersion();
  String payload;
  serializeJson(doc, payload);
  return payload;
}

String wifiInfoPayload() {
  StaticJsonDocument<256> doc;
  if (wifi.isConnected()) {
    JsonObject wifiObj = doc.createNestedObject("Wifi");
    wifiObj["SSID"] = WiFi.SSID();
    wifiObj["BSSId"] = WiFi.BSSIDstr();
    wifiObj["Channel"] = WiFi.channel();
    wifiObj["RSSI"] = WiFi.RSSI();
  } else {
    doc["Wifi"] = "Disconnected";
  }
  
  String payload;
  serializeJson(doc, payload);
  return payload;
}

void syncRelayLed() {
  if (relay.isOn()) {
    blueLed.on();
  } else {
    blueLed.off();
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
  pushDebug("boot", "start fw=" + String(FW_VERSION_STRING));

  EEPROM.begin(OTA_MARKER_EEPROM_SIZE);
  button.begin();
  relay.begin();
  blueLed.begin();
  syncRelayLed();

  // Register local button control before WiFi/OTA so the plug stays usable
  // even when network or OTA checks are slow.
  button.listen({{
      .event = LOW,
      .action = []() {
        pushDebug("button", "pressed");
        if (relay.isOn()) {
          relay.off();
        } else {
          relay.on();
        }
        syncRelayLed();
        pushDebug("relay", relay.isOn() ? "on" : "off");
        publishStatus();
        pushDebug("mqtt", "status_publish_from_button");
      },
  }});
  button.onFalling([]() { registerWifiConfigPress(); });

  // --- Boot sequence with non-blocking delays for LED blinking ---
  auto nonBlockingDelay = [&](unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
      const unsigned long now = millis();
      blueLed.update(now);
      relay.update(now);
      delay(1); // yield to system
    }
  };

  Serial.println("Connecting to WiFi...");
  pushDebug("wifi", "begin ssid=" + String(WIFI_SSID));
  blueLed.blink(500);
  wifi.begin([&]() {
    blueLed.update(millis());
    relay.update(millis());
  }, false, [&]() { blueLed.blink(1000); }); // AP mode should blink at 1s intervals
  pushDebug("wifi", wifi.isConnected() ? "connected ssid=" + WiFi.SSID() : "connect_timeout");

  Serial.println("Initializing UUID and Time...");
  blueLed.blink(100);
  nonBlockingDelay(1000);
  uuid.begin();
  controlTopic = "smart_plug/" + uuid.load();
  debugTopic = controlTopic + "/debug";
  dateTime.begin();
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
  if (otaMarker.length()) {
    pushDebug("ota", "previous_boot=" + otaMarker);
  }
  writeBootState(static_cast<uint32_t>(FW_VERSION_CODE), familyCode);
  clearOtaMarker();
  pushDebug("boot", "uuid=" + uuid.load());
  pushDebug("ota", "boot_source_hint=" + bootSourceHint);

  mqtt.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
  mqtt.subscribe((controlTopic + "/command").c_str());
  pushDebug("mqtt", "begin host=" + String(MQTT_HOST) + ":" + String(MQTT_PORT));

  // OTA 전 MQTT 통신이 연결될 수 있도록 최대 5초 대기
  unsigned long mqttStart = millis();
  while (!mqtt.connected() && millis() - mqttStart < 5000) {
    mqtt.loop();
    nonBlockingDelay(10);
  }

  Serial.println("Checking for firmware updates...");
  pushDebug("ota", "check_start");
  blueLed.blink(250);
  nonBlockingDelay(1000); // Wait a bit to show the new blink pattern
  updater.setDebugLogger([](const String& message) { pushDebug("ota", message); });
  updater.setBootSourceHint(bootSourceHint);
  updater.performFirmwareUpdate(
      [](float progress) {
        Serial.printf("Update Progress: %.2f%%\n", progress * 100.0f);
      },
      [](FirmwareUpdateResult result) {
        Serial.printf("Update Result: %d\n", (int)result);
        pushDebug("ota", "result=" + String((int)result));
      },
      [&]() {
        blueLed.update(millis());
        relay.update(millis());
        flushOneDebug();
      }); // OTA 진행 중 LED 깜빡임 및 로그 즉시 전송

  mqtt.onReceived([](String topic, String payload) {
    pushDebug("mqtt", "rx " + payload);
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
      pushDebug("mqtt", "rx_invalid_json");
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
      pushDebug("relay", "on");
      publishStatus();
      pushDebug("mqtt", "cmd_on");
    } else if (cmd == "off") {
      relay.off();
      syncRelayLed();
      pushDebug("relay", "off");
      publishStatus();
      pushDebug("mqtt", "cmd_off");
    } else if (cmd == "status") {
      publishStatus();
      pushDebug("mqtt", "cmd_status");
    }
  });

  syncRelayLed();
  publishStatus();
  publishWifiInfo();
  pushDebug("boot", "setup_done");
}

void loop() {
  const unsigned long now = millis();

  wifi.loop();
  mqtt.loop();
  dateTime.update();
  button.update(now);
  blueLed.update(now);
  relay.update(now);

  if (wifiConfigPressCount > 0 && (now - wifiConfigSequenceStartMs) > WIFI_CONFIG_TRIGGER_WINDOW_MS) {
    wifiConfigPressCount = 0;
    wifiConfigSequenceStartMs = 0;
  }

  if (wifiConfigRequested) {
    wifiConfigRequested = false;
    pushDebug("wifi", "config_portal_requested_6_clicks");
    Serial.println("WiFi setup requested by 6 button presses");
    wifi.resetSettings();
    blueLed.blink(1000);
    const bool configured = wifi.openConfigPortal([&]() {
      blueLed.update(millis());
      relay.update(millis());
      flushOneDebug();
    }, [&]() { blueLed.blink(1000); });
    pushDebug("wifi", configured ? "config_portal_connected ssid=" + WiFi.SSID() : "config_portal_failed");
    if (configured) {
      publishWifiInfo();
      publishStatus();
    }
  }

  const bool wifiConnected = wifi.isConnected();
  const bool mqttConnected = mqtt.connected();
  if (wifiConnected != previousWifiConnected) {
    previousWifiConnected = wifiConnected;
    pushDebug("wifi", wifiConnected ? "connected_loop ssid=" + WiFi.SSID() : "disconnected");
  }
  if (mqttConnected != previousMqttConnected) {
    previousMqttConnected = mqttConnected;
    pushDebug("mqtt", mqttConnected ? "connected" : "disconnected");
  }

  // The single blue LED indicates system status.
  // It blinks if not connected, otherwise it shows the relay state.
  // This is a compromise from the original 2-LED (status + relay) design.
  if (!wifiConnected) {
    blueLed.blink(500);
  } else if (!mqttConnected) {
    blueLed.blink(50);
  } else {
    syncRelayLed(); // When fully connected, LED shows relay status
  }

  if (now - lastPublishMs >= STATUS_PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;
    publishStatus();
    publishWifiInfo();
  }

  if (now - lastDebugFlushMs >= DEBUG_FLUSH_INTERVAL_MS) {
    lastDebugFlushMs = now;
    flushOneDebug();
  }

  delay(10);
}
