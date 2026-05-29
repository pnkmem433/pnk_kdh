#include <ArduinoBLE.h>

#define SERVICE_NAME "BLEMultiTest_05"
#define SERVICE_UUID "fd89530d-92d4-4211-b1e9-d1277b603dd3"
#define SERVICE_RXCHAR "75cb2362-a085-4d54-8be5-e92fb31ab001"
#define SERVICE_TXCHAR "d0bf1f15-8bef-4725-b884-da28968fb7ff"
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