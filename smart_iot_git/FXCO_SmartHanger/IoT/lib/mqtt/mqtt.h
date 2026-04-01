#pragma once

#include "mqtt_client.h"
#include <Arduino.h>
#include <functional>

/*
  ==================================================================================
  Mqtt 래퍼 상세 설명
  ==================================================================================
  [목적]
  ESP-IDF의 저수준 mqtt_client API를 프로젝트 코드에서 단순하게 사용하도록 감싼다.

  [제공 기능]
  - begin(): 연결 시작
  - subscribe()/publish(): 토픽 구독/발행
  - onReceived(): 메시지 콜백 등록
  - isConnected(): 현재 상태 조회

  [내부 상태]
  _connected는 MQTT_EVENT_CONNECTED/DISCONNECTED 이벤트로 갱신된다.
  publish()는 _connected == true일 때만 실제 전송 시도한다.
  ==================================================================================
*/

struct MqttServerInfo {
  String server;
  int port;
  String user;
  String password;
  int qos;
};

class Mqtt {
public:
  explicit Mqtt(const MqttServerInfo &info);

  // MQTT 클라이언트 생성 및 연결 시작
  void begin(String name = "");

  // 연결 종료 및 핸들 파괴
  void stop();

  int subscribe(const char *topic);
  int unsubscribe(const char *topic);
  int publish(const String &topic, const String &message, bool retain = false);

  // 수신 데이터(topic, payload) 콜백 등록
  void onReceived(std::function<void(String, String)> cb);

  // 현재 연결 상태
  bool isConnected();

private:
  static void mqttEventHandler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data);

  // 이름을 지정하지 않았을 때 사용하는 기본 client id 생성
  String makeClientId() const;

private:
  MqttServerInfo _info;
  esp_mqtt_client_handle_t _client = nullptr;
  std::function<void(String, String)> _messageCallback;

  bool _connected = false;
};
