#include <ArduinoBLE.h>

#define SERVICE_NAME "BLEMultiTest_02"
#define SERVICE_UUID "10d99883-46e1-486c-8dc6-f1761cd3a969"
#define SERVICE_RXCHAR "4f95c09a-f6ad-45e3-8afb-7e9e4afd10e0"
#define SERVICE_TXCHAR "57e4dfaf-d4f4-4789-9f0f-9d14672942e9"
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