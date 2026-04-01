#ifndef BLE_MANAGER_X_H
#define BLE_MANAGER_X_H

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <functional>

#include <../struct/bleCredential.h>

struct BleManagerXBeginParams {
  BleCredential bleCredential;
  std::function<void(void)> onConnect;
  std::function<void(void)> onDisconnect;
  std::function<void(String)> onDataReceive;
};

class BleManagerX {
private:
  BleCredential bleCredential;

  BLEService *_service;
  BLEStringCharacteristic *_rxChar;
  BLEStringCharacteristic *_txChar;

  BLEDevice central;

  BleManagerXBeginParams bleManagerXBeginParams;

public:
  BleManagerX();

  bool begin(BleManagerXBeginParams bleManagerXBeginParams);

  bool isconnect();
  void disconnect();

  BleCredential getBleCredential();

  void print(String data);
};

#endif // BLE_MANAGER_X_H
