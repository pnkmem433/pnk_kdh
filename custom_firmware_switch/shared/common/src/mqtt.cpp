#include "mqtt.h"

Mqtt* Mqtt::instance = nullptr;

Mqtt::Mqtt() : client(espClient) { instance = this; }

void Mqtt::begin(const char* server, uint16_t port, const char* user, const char* password) {
  _server = server ? server : "";
  _port = port;
  _user = user ? user : "";
  _pass = password ? password : "";

  client.setServer(_server.c_str(), _port);
  client.setKeepAlive(_keepAlive);
  client.setSocketTimeout(_socketTimeout);
  client.setBufferSize(1024);
  client.setCallback(callback);
}

bool Mqtt::reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  const String clientId = makeClientId();
  bool ok = false;

  if (_user.length() || _pass.length()) {
    ok = client.connect(clientId.c_str(), _user.c_str(), _pass.c_str());
  } else {
    ok = client.connect(clientId.c_str());
  }

  if (ok) {
    for (const auto& topic : _topics) {
      client.subscribe(topic.c_str());
    }
  }
  return ok;
}

void Mqtt::loop() {
  if (!client.connected()) {
    if (millis() - _lastReconnectMs < _reconnectDelayMs) {
      return;
    }

    _lastReconnectMs = millis();
    if (reconnect()) {
      Serial.println("Reconnected to MQTT server");
    }
    return;
  }

  client.loop();
}

bool Mqtt::subscribe(const char* topic) {
  if (std::find(_topics.begin(), _topics.end(), String(topic)) == _topics.end()) {
    _topics.push_back(String(topic));
  }

  if (!client.connected()) {
    return false;
  }
  return client.subscribe(topic);
}

void Mqtt::unsubscribe(const char* topic) {
  _topics.erase(std::remove_if(_topics.begin(), _topics.end(),
                               [&](const String& item) { return item == topic; }),
                _topics.end());
  if (client.connected()) {
    client.unsubscribe(topic);
  }
}

bool Mqtt::publish(String topic, String message) {
  if (!client.connected()) {
    return false;
  }
  return client.publish(topic.c_str(), message.c_str());
}

void Mqtt::onReceived(std::function<void(String, String)> callbackFn) {
  messageCallback = std::move(callbackFn);
}

void Mqtt::callback(char* topic, byte* payload, unsigned int length) {
  if (!instance || !instance->messageCallback) {
    return;
  }

  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; ++i) {
    message += static_cast<char>(payload[i]);
  }
  instance->messageCallback(String(topic), message);
}

String Mqtt::makeClientId() const {
#if defined(ESP8266)
  return "esp02s_" + String(ESP.getChipId(), HEX) + "_" + String(millis());
#else
  uint64_t mac = ESP.getEfuseMac();
  char buf[48];
  snprintf(buf, sizeof(buf), "esp32_%04X%04X_%lu", (uint16_t)(mac >> 32),
           (uint32_t)mac, (unsigned long)millis());
  return String(buf);
#endif
}

bool Mqtt::connected() { return client.connected(); }
