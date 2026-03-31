#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include <vector>

class MqttService {
public:
  using MessageCallback = std::function<void(const String&, const String&)>;

  MqttService();

  void begin(const char* host, uint16_t port, const char* user, const char* password);
  void tick(unsigned long retryDelayMs);
  bool connected();
  bool publish(const String& topic, const String& payload, bool retained = false);
  bool subscribe(const String& topic);
  void setCallback(MessageCallback callback);

private:
  static void rawCallback(char* topic, byte* payload, unsigned int length);
  bool reconnect();
  String makeClientId() const;

  WiFiClient wifiClient_;
  PubSubClient client_;
  String host_;
  uint16_t port_ = 1883;
  String user_;
  String password_;
  std::vector<String> subscriptions_;
  unsigned long lastAttemptMs_ = 0;
  MessageCallback callback_;

  static MqttService* instance_;
};
