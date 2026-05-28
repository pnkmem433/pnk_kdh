#include <Arduino.h>
#include <nfc.h>

extern NfcReader nfcReader;
extern NfcReader nfcReader2;
extern TaskRunner muxTask;

namespace {

constexpr uint32_t BAUD = 115200;
constexpr uint8_t TEST_SLAVE_ID = 1;

String g_lastUid1;
String g_lastUid2;

}  // namespace

void setup() {
  Serial.begin(BAUD);
  delay(300);

  Serial.println();
  Serial.println("=== NFC ONLY TEST START ===");
  Serial.printf("[SLAVE TEST] slave id = %u\n", TEST_SLAVE_ID);
  Serial.println("[SLAVE TEST] pin map");
  Serial.println("  NFC1: SS=D3, RST=D2, SPI=D8/D9/D10");
  Serial.println("  NFC2: SS=D4, RST=D2, SPI=D8/D9/D10");

  nfcReader.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid1 = uidStr;
            Serial.printf("[SLAVE %u] NFC1 READ uid=%s\n", TEST_SLAVE_ID, uidStr.c_str());
          },
      .onRemove =
          []() {
            if (g_lastUid1.isEmpty()) {
              return;
            }
            Serial.printf("[SLAVE %u] NFC1 REMOVED last_uid=%s\n", TEST_SLAVE_ID, g_lastUid1.c_str());
            g_lastUid1 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("[SLAVE %u] NFC1 ERROR %s\n", TEST_SLAVE_ID, err.c_str());
          },
  });

  nfcReader2.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid2 = uidStr;
            Serial.printf("[SLAVE %u] NFC2 READ uid=%s\n", TEST_SLAVE_ID, uidStr.c_str());
          },
      .onRemove =
          []() {
            if (g_lastUid2.isEmpty()) {
              return;
            }
            Serial.printf("[SLAVE %u] NFC2 REMOVED last_uid=%s\n", TEST_SLAVE_ID, g_lastUid2.c_str());
            g_lastUid2 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("[SLAVE %u] NFC2 ERROR %s\n", TEST_SLAVE_ID, err.c_str());
          },
  });

  nfcReader.setEnabled(true);
  nfcReader2.setEnabled(false);

  muxTask.begin({
      .loop =
          []() {
            static uint32_t lastSwitchMs = 0;
            static bool firstActive = true;
            const uint32_t now = millis();

            if (now - lastSwitchMs < 200) {
              return;
            }

            lastSwitchMs = now;
            firstActive = !firstActive;
            nfcReader.setEnabled(firstActive);
            nfcReader2.setEnabled(!firstActive);
          },
  });
}

void loop() {}
