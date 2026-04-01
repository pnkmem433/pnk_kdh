/*
  [파일 설명] lib/mqtt/mqtt.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "mqtt.h"

Mqtt::Mqtt(const MqttServerInfo &info) : _info(info) {}

/*
  begin(name)
  - MQTT 클라이언트 구성값을 만들고 연결을 시작한다.
  - 인증 정보(user/password)가 있으면 함께 설정한다.
  - 이벤트 핸들러를 등록해 연결/해제/수신/오류를 비동기로 처리한다.
*/
void Mqtt::begin(String name) {
  esp_mqtt_client_config_t cfg = {};

  // name이 있으면 그대로 쓰고, 없으면 MAC 기반 ID를 생성한다.
  static String clientId = (name == "") ? makeClientId() : name;

  cfg.uri = _info.server.c_str();
  cfg.port = _info.port;
  cfg.client_id = clientId.c_str();

  if (_info.user.length()) {
    cfg.username = _info.user.c_str();
    cfg.password = _info.password.c_str();
  }

  // keepalive를 길게 잡아 불필요한 연결 끊김을 줄인다.
  // 네트워크 품질이 들쭉날쭉한 환경에서 재연결 반복을 완화하는 목적.
  cfg.keepalive = 300;
  cfg.disable_clean_session = false;

  // Last Will:
  // 장치가 비정상 종료되면 broker가 offline 상태를 자동 발행한다.
  // (정상 종료 stop() 경로에서는 broker가 LWT를 발행하지 않는다.)
  cfg.lwt_topic = "device/status";
  cfg.lwt_msg = "offline";
  cfg.lwt_qos = _info.qos;
  cfg.lwt_retain = true;

  _client = esp_mqtt_client_init(&cfg);

  esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, mqttEventHandler,
                                 this);

  esp_mqtt_client_start(_client);
}

/*
  stop()
  - MQTT 연결을 명시적으로 중단하고 내부 핸들을 파괴한다.
  - 재시작하려면 begin()을 다시 호출하면 된다.
*/
void Mqtt::stop() {
  if (_client) {
    esp_mqtt_client_stop(_client);
    esp_mqtt_client_destroy(_client);
    _client = nullptr;
    _connected = false;
  }
}

// subscribe(): topic 구독 요청
int Mqtt::subscribe(const char *topic) {
  return _client ? esp_mqtt_client_subscribe(_client, topic, _info.qos) : -1;
}

// unsubscribe(): topic 구독 해제 요청
int Mqtt::unsubscribe(const char *topic) {
  return _client ? esp_mqtt_client_unsubscribe(_client, topic) : -1;
}

/*
  publish(topic, message, retain)
  - 연결 상태일 때만 실제 발행을 수행한다.
  - 미연결이면 -1을 반환해 호출부가 재시도/무시 정책을 선택할 수 있게 한다.
*/
int Mqtt::publish(const String &topic, const String &message, bool retain) {
  // _connected를 체크해 연결 전 publish 호출을 빠르게 실패 처리한다.
  // 호출부는 반환값(-1 여부)로 실패를 판단할 수 있다.
  return (_client && _connected)
             ? esp_mqtt_client_publish(_client, topic.c_str(), message.c_str(),
                                       message.length(), _info.qos, retain)
             : -1;
}

// onReceived(): 수신 데이터 콜백 등록
void Mqtt::onReceived(std::function<void(String, String)> cb) {
  _messageCallback = std::move(cb);
}

/*
  mqttEventHandler(...)
  - MQTT_EVENT_CONNECTED/DISCONNECTED: 내부 연결 상태 플래그 갱신
  - MQTT_EVENT_DATA: 등록된 콜백으로 topic/payload 전달
  - MQTT_EVENT_ERROR: 진단 로그 출력
*/
void Mqtt::mqttEventHandler(void *handler_args, esp_event_base_t,
                            int32_t, void *event_data) {
  auto *self = static_cast<Mqtt *>(handler_args);
  auto *event = (esp_mqtt_event_handle_t)event_data;

  Serial.printf("[MQTT] Event id: %d\n", event->event_id);

  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    // 연결 완료: 이후 subscribe/publish가 정상 동작한다.
    self->_connected = true;
    Serial.println("[MQTT] Connected");
    break;

  case MQTT_EVENT_DISCONNECTED:
    // 연결 해제: publish는 즉시 -1 반환하게 된다.
    self->_connected = false;
    Serial.println("[MQTT] Disconnected");
    break;

  case MQTT_EVENT_ERROR:
    // 오류 원인을 최대한 로그로 남겨 현장 디버깅 가능성을 높인다.
    Serial.println("[MQTT] ERROR");
    if (event->error_handle) {
      Serial.printf("  esp-tls error: %d\n",
                    event->error_handle->esp_tls_last_esp_err);
      Serial.printf("  tls stack error: %d\n",
                    event->error_handle->esp_tls_stack_err);
      Serial.printf("  socket errno: %d\n",
                    event->error_handle->esp_transport_sock_errno);
    }
    break;

  case MQTT_EVENT_DATA:
    // topic/data 버퍼는 길이 기반이므로 String 생성 시 길이를 명시한다.
    if (self->_messageCallback) {
      String topic(event->topic, event->topic_len);
      String data(event->data, event->data_len);
      self->_messageCallback(topic, data);
    }
    break;

  default:
    break;
  }
}

// makeClientId(): MAC 기반 기본 client id 생성
String Mqtt::makeClientId() const {
  uint64_t mac = ESP.getEfuseMac();
  char buf[32];
  snprintf(buf, sizeof(buf), "esp32_%04X%08X", (uint16_t)(mac >> 32),
           (uint32_t)mac);
  return String(buf);
}

// isConnected(): 최신 이벤트 기준 연결 플래그 반환
bool Mqtt::isConnected() { return _connected; }

