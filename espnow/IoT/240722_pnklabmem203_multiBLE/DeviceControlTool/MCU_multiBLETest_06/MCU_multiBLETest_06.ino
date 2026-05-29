#include <ArduinoBLE.h>

#define SERVICE_NAME "BLEMultiTest_06"
#define SERVICE_UUID "0e71b325-a05f-4336-9156-d3ee1bb3f6c3"
#define SERVICE_RXCHAR "fcd0cc37-8a44-43b1-bfed-fd94c597a6d4"
#define SERVICE_TXCHAR "4e7661ba-afee-4384-be1e-3a6a631135d4"
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