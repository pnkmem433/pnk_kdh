#include <Arduino.h>
#include <Display.h>
#include <HexFormat.h>
#include <I2CSlave.h>
#include <Logger.h>
#include <nfc.h>

extern I2CSlave i2cSlave;
extern NfcReader nfcReader;
extern Display displayUnit;

void setup() {
  Serial.begin(115200);
  delay(200);

  Logger::print({.value = "nfc_slave: I2C 슬레이브 시작"});

  displayUnit.begin();

  i2cSlave.begin({
      .onReceive =
          [](const String& echoStr) {
            Logger::print({.value = "%s", .args = {echoStr.c_str()}});
            displayUnit.stopLoading();
            displayUnit.showText({.title = "UID (echo)", .value = echoStr.c_str()});
          },
      .intervalMs = 50,
      .startDelayMs = 0,
  });

  nfcReader.begin({
      .onRead =
          [](const String& uidStr) {
            Logger::print({.value = "NFC 카드 인식: %s", .args = {uidStr.c_str()}});
            displayUnit.startLoading({.title = "UID (echo)", .value = uidStr.c_str()});
            i2cSlave.send({.value = uidStr});
          },
      .onRemove =
          []() {
            Logger::print({.value = "NFC 카드 제거"});
            displayUnit.showText({.title = "UID (echo)", .value = "waiting..."});
            i2cSlave.send({.value = ""});
          },
      .onError = [](const String& err) { Logger::print({.value = "NFC 오류: %s", .args = {err.c_str()}}); },
  });

  String versionStr = HexFormat::toString({.value = nfcReader.version(), .width = 2, .upper = true, .prefix = true});
  String addressStr = HexFormat::toString({.value = i2cSlave.address(), .width = 2, .upper = true, .prefix = true});

  displayUnit.showText({
      .title = "Addr/Ver",
      .value = String("Addr=" + addressStr + "\nVer=" + versionStr).c_str(),   
  });
  delay(5000);

  displayUnit.showText({.title = "UID (echo)", .value = "waiting..."});
}

void loop() {}
