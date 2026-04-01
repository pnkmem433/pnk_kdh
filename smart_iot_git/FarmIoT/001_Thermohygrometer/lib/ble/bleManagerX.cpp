#include "bleManagerX.h"

BleManagerX::BleManagerX()
    : _service(nullptr), _rxChar(nullptr), _txChar(nullptr) {}

bool BleManagerX::begin(BleManagerXBeginParams bleManagerXBeginParams) {
  bleCredential = bleManagerXBeginParams.bleCredential;

  _service = new BLEService(bleCredential.uuid.c_str());
  _rxChar = new BLEStringCharacteristic(bleCredential.rxChar.c_str(),
                                        BLEWriteWithoutResponse | BLEWrite, 20);
  _txChar =
      new BLEStringCharacteristic(bleCredential.txChar.c_str(), BLENotify, 20);

  BLE.begin();

  BLE.setLocalName(bleCredential.name.c_str());
  BLE.setAdvertisedService(*_service);

  _service->addCharacteristic(*_rxChar);
  _service->addCharacteristic(*_txChar);

  BLE.addService(*_service);
  BLE.advertise();

  this->bleManagerXBeginParams = bleManagerXBeginParams;
  xTaskCreate(
      [](void *parameter) {
        BleManagerX *manager = static_cast<BleManagerX *>(parameter);

        bool preState = false;
        BLEDevice central;

        while (1) {
          if (BLE.connected()) {
            if (!preState)
              manager->bleManagerXBeginParams.onConnect();
            if (manager->_rxChar->valueLength() > 0) {
              String receivedData = manager->_rxChar->value();
              manager->_rxChar->setValue("");
              manager->bleManagerXBeginParams.onDataReceive(receivedData);
            }
          } else {
            if (preState)
              manager->bleManagerXBeginParams.onDisconnect();
            central = BLE.central();
          }

          preState = BLE.connected();
          vTaskDelay(10 / portTICK_PERIOD_MS);
        }
      },
      "bleTask", 10000, this, 1, NULL);

  Serial.println("BLE server started");
  return true;
}

bool BleManagerX::isconnect() { return BLE.connected(); }
void BleManagerX::disconnect() { BLE.disconnect(); }

void BleManagerX::print(String data) {
  Serial.println(data);
  if (BLE.connected())
    _txChar->writeValue(data.c_str());
}

BleCredential BleManagerX::getBleCredential() { return bleCredential; }