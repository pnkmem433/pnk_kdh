#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include <vector>
#include <algorithm>

class Mqtt {
public:
  Mqtt();

  void begin(const char* server, const char* user=nullptr, const char* password=nullptr);

  // 구독/해제 (구독 성공 여부 반환)
  bool subscribe(const char* topic);
  void unsubscribe(const char* topic);

  bool publish(String topic, String message);

  void onReceived(std::function<void(String, String)> callback);

  bool connected();

private:
  static void callback(char* topic, byte* payload, unsigned int length);
  static void loopTask(void* pvParameters);

  void loop();
  bool reconnect();               // ⬅ 재접속 + 재구독
  String makeClientId() const;    // ⬅ 유니크 ClientID

private:
  WiFiClient   espClient;
  PubSubClient client;

  String _server, _user, _pass;
  uint16_t _port = 1883;
  uint16_t _keepAlive = 60;
  uint16_t _socketTimeout = 30;

  static Mqtt* instance;
  std::function<void(String, String)> messageCallback;

  std::vector<String> _topics;    // ⬅ 구독 목록 저장 (재연결 시 재구독)
  uint32_t _reconnectDelayMs = 3000;
};
