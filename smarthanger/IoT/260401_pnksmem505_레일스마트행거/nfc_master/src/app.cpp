#include <Arduino.h>
#include <HexFormat.h>
#include <I2CMaster.h>
#include <Logger.h>

extern I2CMaster i2cMaster;

void setup() {
  Serial.begin(115200);
  delay(200);

  Logger::print({.value = "nfc_master: I2C 마스터 시작"});

  i2cMaster.begin({
      .onDataReceive = [](const I2CMaster::LinkData& data) -> String {
        Logger::print({
            .value = "slave %s: %s",
            .args =
                {
                    HexFormat::toString({.value = data.address, .width = 2, .upper = true, .prefix = true}).c_str(),
                    data.value.c_str(),
                },
        });

        return data.value;  // 반환된 문자열을 자동으로 에코 전송
      },
      .intervalMs = 500,
      .startDelayMs = 200,
  });
}

void loop() {
  // 비워둠(실제 작업은 태스크에서 수행)
}
