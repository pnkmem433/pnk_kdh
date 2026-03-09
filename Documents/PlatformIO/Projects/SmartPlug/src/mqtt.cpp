#include "mqtt.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

Mqtt *Mqtt::instance = nullptr;

Mqtt::Mqtt() : client(espClient) { instance = this; }

void Mqtt::begin(const char *server, const char *user, const char *password) {
  _server = server ? server : "";
  _user = user ? user : "";
  _pass = password ? password : "";

  client.setServer(_server.c_str(), _port);
  client.setKeepAlive(_keepAlive);
  client.setSocketTimeout(_socketTimeout);
  client.setBufferSize(1024); // ⬅ 버퍼 확장 (중요)
  client.setCallback(callback);

  // 백그라운드 루프/재접속 태스크
  xTaskCreate(loopTask, "MQTTLoopTask", 9182, this, 1, nullptr);
}

bool Mqtt::reconnect() {
  if(WiFi.status() != WL_CONNECTED) return 0;


  // 유니크 ClientID: MAC + millis
  String clientId = makeClientId();

  bool ok = false;

  if (_user.length() || _pass.length())
    ok = client.connect(clientId.c_str(), _user.c_str(), _pass.c_str());
  else
    ok = client.connect(clientId.c_str());

  if (ok) {
    // 저장된 모든 토픽 재구독
    for (auto &t : _topics) {
      bool subOk = client.subscribe(t.c_str());
      if (!subOk)
        Serial.printf("resubscribe('%s') failed\n", t.c_str());
    }
  }
  return ok;
}

bool Mqtt::subscribe(const char *topic) {
  // 구독 목록에 저장(중복 방지)
  if (std::find(_topics.begin(), _topics.end(), String(topic)) == _topics.end())
    _topics.push_back(String(topic));

  if (!client.connected()) {
    Serial.println("subscribe(): not connected yet; will resub on reconnect");
    return false;
  }
  bool ok = client.subscribe(topic);
  Serial.printf("subscribe('%s') -> %s\n", topic, ok ? "OK" : "FAIL");
  return ok;
}

void Mqtt::unsubscribe(const char *topic) {
  _topics.erase(std::remove_if(_topics.begin(), _topics.end(),
                               [&](const String &s) { return s == topic; }),
                _topics.end());
  if (client.connected())
    client.unsubscribe(topic);
}

bool Mqtt::publish(String topic, String message) {
  if (!client.connected()) {
    Serial.println("publish(): not connected");
    return false;
  }
  bool ok = client.publish(topic.c_str(), message.c_str());
  if (!ok)
    Serial.printf("publish('%s') failed, rc=%d\n", topic.c_str(),
                  client.state());
  return ok;
}

void Mqtt::onReceived(std::function<void(String, String)> cb) {
  messageCallback = std::move(cb);
}

void Mqtt::callback(char *topic, byte *payload, unsigned int length) {
  if (instance && instance->messageCallback) {
    String t(topic);
    String m;
    m.reserve(length);
    for (unsigned int i = 0; i < length; i++)
      m += (char)payload[i];
    instance->messageCallback(t, m);
  }
}

void Mqtt::loop() {
  // 끊기면 주기적으로 재연결
  if (!client.connected()) {
    if (reconnect()) {
      Serial.println("Reconnected to MQTT server");
    } else {
      vTaskDelay(pdMS_TO_TICKS(_reconnectDelayMs));
      return;
    }
  }
  client.loop();
}

void Mqtt::loopTask(void *pvParameters) {
  auto self = static_cast<Mqtt *>(pvParameters);
  for (;;) {
    self->loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

String Mqtt::makeClientId() const {
  uint64_t mac = ESP.getEfuseMac();
  char buf[48];
  snprintf(buf, sizeof(buf), "esp32_%04X%04X_%lu", (uint16_t)(mac >> 32),
           (uint32_t)mac, (unsigned long)millis());
  return String(buf);
}

bool Mqtt::connected() { return client.connected(); }