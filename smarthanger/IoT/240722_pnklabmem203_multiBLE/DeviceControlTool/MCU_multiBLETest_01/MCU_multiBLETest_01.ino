#include <ArduinoBLE.h>

#define SERVICE_NAME "BLEMultiTest_01"
#define SERVICE_UUID "92c968b1-d59b-41f3-bf8e-a2bd383dc73e"
#define SERVICE_RXCHAR "daa09ad3-d45a-44b5-9624-feac2213f93f"
#define SERVICE_TXCHAR "b419d0e6-a538-4416-8451-e47522a55ee6"
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