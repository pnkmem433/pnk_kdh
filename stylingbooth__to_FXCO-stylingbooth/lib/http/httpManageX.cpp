#include "httpManageX.h"

#include <errno.h>
extern "C" {
#include "lwip/errno.h"
}

SemaphoreHandle_t HttpManageX::_ethernetMutex = NULL;

namespace {
int extractIntField(const JsonDocument &doc, const char *key) {
  if (doc[key].is<int>()) {
    return doc[key].as<int>();
  }

  JsonVariantConst data = doc["data"];
  if (!data.isNull() && data[key].is<int>()) {
    return data[key].as<int>();
  }

  JsonVariantConst result = doc["result"];
  if (!result.isNull() && result[key].is<int>()) {
    return result[key].as<int>();
  }

  JsonVariantConst firstItem = doc[0];
  if (!firstItem.isNull() && firstItem[key].is<int>()) {
    return firstItem[key].as<int>();
  }

  return 0;
}
} // namespace

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
  for (int attempt = 0; attempt < 3; ++attempt) {
    String json = send(true, "/session/last");
    json.trim();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (!error) {
      int seq = extractIntField(doc, "seq");
      if (seq > 0) {
        _session = seq;
        Serial.println("[session]\t/session/last seq=" + String(seq));
        return seq;
      }
    }

    Serial.println("[session]\t/session/last retry=" + String(attempt + 1) +
                   " body=" + json);
    delay(200);
  }

  Serial.println("[session]\tFailed to load /session/last");
  _session = 0;
  return 0;
}

int HttpManageX::loadLoadingTime() {
  for (int attempt = 0; attempt < 3; ++attempt) {
    String json = send(true, "/system-parameter/1");
    json.trim();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (!error) {
      int parameter = extractIntField(doc, "parameter");
      if (parameter > 0) {
        Serial.println("[session]\t/system-parameter/1 parameter=" +
                       String(parameter));
        return parameter;
      }
    }

    Serial.println("[session]\t/system-parameter/1 retry=" +
                   String(attempt + 1) + " body=" + json);
    delay(200);
  }

  Serial.println("[session]\tFailed to load /system-parameter/1");
  return 0;
}

String HttpManageX::send(bool waitResponse, String path, String postData) {
  uint64_t startMillis = esp_timer_get_time();
  uint64_t lastCheckpoint = startMillis;
  String body = "";
  Serial.print("[HTTP]\t");

  // 1. Mutex Lock
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    Serial.println("mutex ?띾뱷 ?ㅽ뙣");
    return "";
  }

  uint64_t now = esp_timer_get_time();
  Serial.print("get mutex(" + String((now - startMillis) / 1000) + "??" +
               String((now - lastCheckpoint) / 1000) + "?? ??\t");
  lastCheckpoint = now;

  vTaskDelay(pdMS_TO_TICKS(50));

  // 2. ?곌껐 ?쒕룄
  Serial.print("connecting ??\t");
  bool connectResult = false;
  IPAddress directIP;
  if (directIP.fromString(_url)) {
    connectResult = client.connect(directIP, _port);
  } else {
    connectResult = client.connect(_url.c_str(), _port);
  }

  if (connectResult) {
    now = esp_timer_get_time();
    Serial.print("connected(" + String((now - startMillis) / 1000) + "??" +
                 String((now - lastCheckpoint) / 1000) + "?? ??\t");
    lastCheckpoint = now;
  } else {
    now = esp_timer_get_time();
    int err = errno;
    Serial.print("fail(" + String((now - startMillis) / 1000) + "??" +
                 String((now - lastCheckpoint) / 1000) + "??\n");

    client.stop(); // ?뚯폆 ?꾩닔 諛⑹???
    xSemaphoreGive(_ethernetMutex);
    return "";
  }

  // 3. ?붿껌 ?꾩넚
  Serial.print("send ??\t");

  if (postData.length() > 0) {
    sendPost(client, path, postData);
  } else {
    sendGet(client, path);
  }

  // 4. ?묐떟 ?꾩슂 ?щ? 泥댄겕
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
  Serial.print("success(" + String((now - startMillis) / 1000) + "??" +
               String((now - lastCheckpoint) / 1000) + "??\n");

  // 5. Mutex Unlock
  xSemaphoreGive(_ethernetMutex);
  client.stop();

  return body;
}

// GET ?붿껌 鍮뚮뱶
void HttpManageX::sendGet(EthernetClient &client, const String &path) {
  client.print("GET " + path + " HTTP/1.1\r\n");
  client.print("Host: " + _url + "\r\n");
  client.print("Connection: close\r\n\r\n");
}

// POST ?붿껌 鍮뚮뱶
void HttpManageX::sendPost(EthernetClient &client, const String &path,
                           const String &data) {
  client.print("POST " + path + " HTTP/1.1\r\n");
  client.print("Host: " + _url + "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("Content-Length: " + String(data.length()) + "\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(data);
}

// ?묐떟 諛붾뵒 異붿텧
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
