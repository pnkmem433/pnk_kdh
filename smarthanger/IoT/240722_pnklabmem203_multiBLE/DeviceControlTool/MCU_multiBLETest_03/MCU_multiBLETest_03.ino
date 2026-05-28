#include <ArduinoBLE.h>

#define SERVICE_NAME "BLEMultiTest_03"
#define SERVICE_UUID "898a1e7c-10be-4c26-a0b0-92a98300e7a6"
#define SERVICE_RXCHAR "1dbd7549-4d70-49c6-957f-dd72cec59da8"
#define SERVICE_TXCHAR "0c27cd56-d68a-4135-8b61-214b39a747e3"
BLEService testservice(SERVICE_UUID);
BLEStringCharacteristic rxChar(SERVICE_RXCHAR, BLEWriteWithoutResponse | BLEWrite, 20);
BLEStringCharacteristic txChar(SERVICE_TXCHAR, BLENotify, 20);
BLEDevice central;

unsigned long preTime;
int count = 0;

void setup() {
  BLE.begin();
  BLE.setLocalName(SERVICE_NAME);
  BLE.setAdvertisedService(testservice);
  testservice.addCharacteristic(rxChar);
  testservice.addCharacteristic(txChar);
  BLE.addService(testservice);
  BLE.advertise();

  Serial.begin(115200);
}

void loop() {
  unsigned long nowTime = millis();
  if (central.connected()) {
    if (nowTime - preTime >= 100) {
      preTime = nowTime;
      count++;
      txChar.writeValue(String(SERVICE_NAME) + ":" + String(count));
    }
  } else {
    count = 0;
    central = BLE.central();
  }
}