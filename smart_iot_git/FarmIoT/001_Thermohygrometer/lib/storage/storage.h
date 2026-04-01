#ifndef WiFiStorage_h
#define WiFiStorage_h

#include <Arduino.h>
#include <LittleFS.h>

#include <esp_system.h>
#include <functional>
#include <vector>

#include "../struct/bleCredential.h"
#include "../struct/thermoHygroData.h"
#include "../struct/wifiCredential.h"

class Storage {
public:
  Storage();
  void begin();

  std::vector<String> loadBleUuids();

  bool addWifi(WifiCredential wifiCredential);
  std::vector<WifiCredential> loadWifis();
  bool resetWiFi();

  void addThermohygrometerData(ThermohygrometerData data);
  bool getThermohygrometerData(
      std::function<bool(std::vector<ThermohygrometerData>)> callbackFunction);
  bool resetThermohygrometerData();

  void fileLock();

private:
  String _bleFileName;
  String _wifiFileName;
  String _thermohygrometerDataFileName;

  void deleteOldData();
  String generateRandomUuidV4();

  SemaphoreHandle_t _fsMutex;

#define FS_LOCK()                                                              \
  xSemaphoreTake(_fsMutex, portMAX_DELAY);                                     \
  delay(100);

#define FS_UNLOCK()                                                            \
  delay(100);                                                                  \
  xSemaphoreGive(_fsMutex);
};

#endif
