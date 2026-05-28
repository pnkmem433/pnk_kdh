#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <PubSubClient.h>
#include <algorithm>
#include <functional>
#include <vector>

class Mqtt {
public:
  Mqtt();

  void begin(const char* server, uint16_t port = 1883, const char* user = nullptr, const char* password = nullptr);
  void loop();
  bool subscribe(const char* topic);
  void unsubscribe(const char* topic);
  bool publish(String topic, String message);
  void onReceived(std::function<void(String, String)> callback);
  bool connected();

private:
  static void callback(char* topic, byte* payload, unsigned int length);
  bool reconnect();
  String makeClientId() const;

  WiFiClient espClient;
  PubSubClient client;

  String _server, _user, _pass;
  uint16_t _port = 1883;
  uint16_t _keepAlive = 60;
  uint16_t _socketTimeout = 30;
  uint32_t _reconnectDelayMs = 3000;
  unsigned long _lastReconnectMs = 0;

  static Mqtt* instance;
  std::function<void(String, String)> messageCallback;
  std::vector<String> _topics;
};

#endif
