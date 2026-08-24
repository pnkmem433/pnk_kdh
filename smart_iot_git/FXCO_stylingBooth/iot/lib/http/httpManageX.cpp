#include "httpManageX.h"

#include <errno.h>
extern "C" {
#include "lwip/errno.h"
}

SemaphoreHandle_t HttpManageX::_ethernetMutex = NULL;

void HttpManageX::initMutex() {
  if (_ethernetMutex == NULL) {
    _ethernetMutex = xSemaphoreCreateMutex();
  }
}

HttpManageX::HttpManageX(String url, uint16_t port)
    : _url(url), _port(port), _session(0) {}

void HttpManageX::closeFittingRoomDoor() {
  if (_session == 0)
    setSession();

  send(false, "/fitting-room-door-sensor",
       "{\"session\": " + String(_session) + ",\"is_opened\": 0}");
}

void HttpManageX::openFittingRoomDoor() {
  send(false, "/session/deactivate-door-open", "{}");
  send(false, "/fitting-room-door-sensor",
       "{\"session\": " + String(_session) + ",\"is_opened\": 1}");
}

void HttpManageX::checkPeopleInsideFittingRoom(bool pir_sensor) {
  String json =
      send(true, "/session/activate-door-close",
           "{\"pir_status\": " + String(pir_sensor ? "1" : "0") + "}");

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  int result = doc["seq"];

  if (result > 0) {
    _session = result;
  }
}

void HttpManageX::closeShowroomDoor() {
  if (_session == 0)
    setSession();
  send(false, "/video-room-door-sensor",
       "{\"session\": " + String(_session) + ",\"is_opened\": 0}");
}

void HttpManageX::openShowroomDoor() {
  if (_session == 0)
    setSession();
  send(false, "/video-room-door-sensor",
       "{\"session\": " + String(_session) + ",\"is_opened\": 1}");
}

void HttpManageX::sendTag(String tags) {
  if (_session == 0)
    setSession();
  send(false, "/rfid-scan",
       "{\"session_seq\": " + String(_session) +
           " ,\"clothes_product_id\": \"" + tags +
           "\",\"scan_source_type\": 0}");
}

int HttpManageX::setSession() {
  String json = send(true, "/session/last");

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  int seq = doc["seq"];

  _session = seq;

  return seq;
}

int HttpManageX::loadLoadingTime() {
  String json = send(true, "/system-parameter/1");

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  int seq = doc["parameter"];

  return seq;
}

String HttpManageX::send(bool waitResponse, String path, String postData) {
  uint64_t startMillis = esp_timer_get_time();
  uint64_t lastCheckpoint = startMillis;
  String body = "";
  Serial.print("[http]\t");

  // 1. Mutex Lock
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    Serial.println("[Error] Failed to acquire Ethernet mutex");
    return "";
  }

  uint64_t now = esp_timer_get_time();
  Serial.print("get mutex(" + String((now - startMillis) / 1000) + "㎳/" +
               String((now - lastCheckpoint) / 1000) + "㎳) → \t");
  lastCheckpoint = now;

  vTaskDelay(pdMS_TO_TICKS(50));

  // 2. 연결 시도
  Serial.print("connecting → \t");
  if (client.connect(_url.c_str(), _port)) {
    now = esp_timer_get_time();
    Serial.print("connected(" + String((now - startMillis) / 1000) + "㎳/" +
                 String((now - lastCheckpoint) / 1000) + "㎳) → \t");
    lastCheckpoint = now;
  } else {
    now = esp_timer_get_time();
    int err = errno;
    Serial.print("fail(" + String((now - startMillis) / 1000) + "㎳/" +
                 String((now - lastCheckpoint) / 1000) + "㎳)\n");

    xSemaphoreGive(_ethernetMutex);
    return "";
  }

  // 3. 요청 전송
  Serial.print("send → \t");

  if (postData.length() > 0) {
    sendPost(client, path, postData);
  } else {
    sendGet(client, path);
  }

  // 4. 응답 필요 여부 체크
  if (waitResponse) {
    client.setTimeout(500);
    String response = client.readString();
    body = parseHttpBody(response);
  } else {
    delay(100);
    while (client.available()) {
      client.read();
    }
  }

  now = esp_timer_get_time();
  Serial.print("success(" + String((now - startMillis) / 1000) + "㎳/" +
               String((now - lastCheckpoint) / 1000) + "㎳)\n");

  // 5. Mutex Unlock
  xSemaphoreGive(_ethernetMutex);
  client.stop();

  return body;
}

// GET 요청 빌드
void HttpManageX::sendGet(EthernetClient &client, const String &path) {
  client.print("GET " + path + " HTTP/1.1\r\n");
  client.print("Host: " + _url + "\r\n");
  client.print("Connection: close\r\n\r\n");
}

// POST 요청 빌드
void HttpManageX::sendPost(EthernetClient &client, const String &path,
                           const String &data) {
  client.print("POST " + path + " HTTP/1.1\r\n");
  client.print("Host: " + _url + "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("Content-Length: " + String(data.length()) + "\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(data);
}

// 응답 바디 추출
String HttpManageX::parseHttpBody(const String &response) {
  int idx = response.indexOf("\r\n\r\n");
  if (idx == -1)
    return response;
  return response.substring(idx + 4);
}

EthernetLinkStatus HttpManageX::safeLinkStatus() {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return Unknown;
  }
  vTaskDelay(pdMS_TO_TICKS(100));
  EthernetLinkStatus status = UIPEthernet.linkStatus();
  UIPEthernet.maintain();
  vTaskDelay(pdMS_TO_TICKS(100));
  xSemaphoreGive(_ethernetMutex);
  return status;
}
