#ifndef HTTPMANAGEX_H
#define HTTPMANAGEX_H

#include "UIPEthernet.h"
#include <ArduinoJson.h>

class HttpManageX {
private:
  String _url;
  uint16_t _port;
  int _session;
  static SemaphoreHandle_t _ethernetMutex; // Mutex 추가
  EthernetClient client;

  String send(bool waitResponse, String path, String postData = "");
  void sendGet(EthernetClient &client, const String &path);
  void sendPost(EthernetClient &client, const String &path, const String &data);
  String parseHttpBody(const String &response);

public:
  HttpManageX(String url, uint16_t port);

  void closeFittingRoomDoor();
  void openFittingRoomDoor();
  void checkPeopleInsideFittingRoom(bool pir_sensor);

  void closeShowroomDoor();
  void openShowroomDoor();

  EthernetLinkStatus safeLinkStatus();

  void sendTag(String tags);

  int setSession();

  int loadLoadingTime();

  static void initMutex(); // Mutex 초기화용
};

#endif
