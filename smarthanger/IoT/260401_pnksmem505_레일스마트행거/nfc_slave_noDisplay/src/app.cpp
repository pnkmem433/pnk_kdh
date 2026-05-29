#include <Arduino.h>
#include <I2CSlave.h>
#include <Logger.h>
#include <nfc.h>

extern I2CSlave i2cSlave;
extern NfcReader nfcReader;
extern NfcReader nfcReader2;
extern TaskRunner muxTask;

// 제거 로그/전송 디바운스용 상태
static String g_lastUid1 = "";
static String g_lastUid2 = "";
static uint32_t g_lastRead1 = 0;
static uint32_t g_lastRead2 = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  Logger::print({.value = "nfc_slave: I2C 슬레이브 시작"});

  i2cSlave.begin({
      .onReceive =
          [](const String& echoStr) {
            Logger::print({.value = "%s", .args = {echoStr.c_str()}});
          },
      .intervalMs = 50,
      .startDelayMs = 0,
  });

  nfcReader.begin({
      .onRead =
          [](const String& uidStr) {
            uint32_t now = millis();
            g_lastUid1 = uidStr;
            Logger::print({.value = "NFC1 card: %s", .args = {uidStr.c_str()}});
            i2cSlave.send({.value = String("1:") + uidStr});
            g_lastRead1 = now;
          },
      .onRemove =
          []() {
            uint32_t now = millis();
            // 최근 카드가 없었으면 제거로 처리하지 않음
            if (g_lastUid1.isEmpty()) {
              return;
            }
            Logger::print({.value = "NFC1 removed"});
            i2cSlave.send({.value = "1:"});
            g_lastUid1 = "";
          },
      .onError = [](const String& err) { Logger::print({.value = "NFC1 error: %s", .args = {err.c_str()}}); },
  });

  nfcReader2.begin({
      .onRead =
          [](const String& uidStr) {
            uint32_t now = millis();
            g_lastUid2 = uidStr;
            Logger::print({.value = "NFC2 card: %s", .args = {uidStr.c_str()}});
            i2cSlave.send({.value = String("2:") + uidStr});
            g_lastRead2 = now;
          },
      .onRemove =
          []() {
            uint32_t now = millis();
            // 최근 카드가 없었으면 제거로 처리하지 않음
            if (g_lastUid2.isEmpty()) {
              return;
            }
            Logger::print({.value = "NFC2 removed"});
            i2cSlave.send({.value = "2:"});
            g_lastUid2 = "";
          },
      .onError = [](const String& err) { Logger::print({.value = "NFC2 error: %s", .args = {err.c_str()}}); },
  });

  // 교대 스캔 초기 상태 설정
  nfcReader.setEnabled(true);
  nfcReader2.setEnabled(false);

  muxTask.begin({
      .loop =
          []() {
            static uint32_t lastSwitchMs = 0;
            static bool firstActive = true;
            uint32_t now = millis();

            // 양쪽 모두 없거나 비슷하면 교대 스캔
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
