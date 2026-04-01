#ifndef HTTPMANAGEX_H
#define HTTPMANAGEX_H

#include <../struct/thermoHygroData.h>
#include <HTTPClient.h>
#include <WiFi.h>

class HttpManageX {
private:
  String _url;
  String _url2;
  String _uuid;

public:
  HttpManageX(String url, String _url2);
  void setUuid(String uuid);

  bool saveDataArray(std::vector<ThermohygrometerData> &dataVector);
  bool saveData(ThermohygrometerData data);

  bool sendControlLog(int controlType);
};

#endif
