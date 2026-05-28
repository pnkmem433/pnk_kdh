/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - ESP-IDF MQTT 래퍼 타입/계약 정의.
*/
#ifndef MQTT_LITE_H
#define MQTT_LITE_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>
#include <ArduinoJson.h>
#include "mqtt_client.h"

/*
  MqttConfig
  - 브로커 접속/인증/TLS/자동재연결 정책을 정의한다.
*/
struct MqttConfig {
    String host;
    int port;
    String user;
    String password;
    bool useTls;
    String caCert;
    bool autoReconnect;
};

struct MqttMessage {
    String topic;
    String raw;
    bool isJson;
    JsonDocument doc;
    JsonVariant json;
    int qos;
    bool retain;
    int messageId;
};

struct MqttSubscribeConfig {
    String topic;
    int qos;
    std::function<void(const MqttMessage&)> callback;
};

/*
  MqttPublishConfig
  - retry/timeoutMs로 QoS1 ACK 대기 재시도 전략을 정의한다.
  - 폭주 완화: 구현부에서 exponential backoff + jitter 적용.
*/
struct MqttPublishConfig {
    String topic;
    String payload;
    int qos;
    bool retain;

    int retry;
    uint32_t timeoutMs;

    std::function<void()> onAck;
    std::function<void()> onFail;
};

class Mqtt {
public:
    /*
      Mqtt(cfg)
      - 입력: 브로커/인증/재연결 정책
      - 출력: 객체 생성
      - 부작용: 설정값 내부 보관
    */
    Mqtt(MqttConfig cfg);

    /*
      begin()
      - 입력/출력: 없음
      - 부작용: ESP-IDF MQTT client 생성 및 이벤트 핸들러 등록
      - 주의: connect()를 호출해야 실제 세션 시작
    */
    void begin();

    /*
      connect()
      - 입력/출력: 없음
      - 부작용: MQTT 네트워크 세션 시작
      - 주의: begin()이 선행되어야 한다.
    */
    void connect();

    /*
      disconnect()
      - 입력/출력: 없음
      - 부작용: MQTT 세션 중지
      - 주의: pending QoS1 메시지는 연결 해제 시 ACK를 받지 못할 수 있다.
    */
    void disconnect();

    /*
      isConnected()
      - 출력: 현재 세션 연결 여부
      - 부작용: 없음
    */
    bool isConnected() const;

    /*
      setAutoReconnect(value)
      - 입력: 자동 재연결 사용 여부
      - 출력: 없음
      - 부작용: 내부 설정값 변경
      - 주의: 이미 생성된 클라이언트 설정에 즉시 반영되지 않을 수 있다.
    */
    void setAutoReconnect(bool value);

    // 연결/해제 이벤트 콜백 등록
    void onConnect(std::function<void(bool)> cb);
    void onDisconnect(std::function<void()> cb);

    /*
      onReceived(cfg)
      - 입력: 구독 토픽, QoS, 콜백
      - 부작용: 내부 구독 목록에 추가, 연결 중이면 즉시 subscribe
      - 주의: 현재 구현은 MQTT_EVENT_DATA 수신 시 등록 콜백들을 호출한다.
    */
    void onReceived(MqttSubscribeConfig cfg);

    /*
      publish(cfg)
      - 입력: 토픽/payload/QoS/retry/timeout 콜백
      - 부작용: QoS1인 경우 pending map에 넣고 ACK 추적/재시도
      - 실패/주의:
        - 미연결 상태면 즉시 반환(전송 안 함)
        - 재시도 한도 초과 시 onFail 호출 후 pending 제거
    */
    void publish(MqttPublishConfig cfg);

private:
    esp_mqtt_client_handle_t _client = nullptr;
    MqttConfig _config;

    bool _connected = false;
    bool _hasEverConnected = false;

    std::vector<MqttSubscribeConfig> _subs;

    struct PendingMessage {
        String topic;
        String payload;
        int qos;
        bool retain;

        int retryCount = 0;
        int maxRetry;
        uint32_t baseTimeout;
        uint32_t nextRetryAt;

        std::function<void()> onAck;
        std::function<void()> onFail;
    };

    std::map<int, PendingMessage> _pending;

    std::function<void(bool)> _onConnect;
    std::function<void()> _onDisconnect;

    static void _eventHandler(void* handler_args,
                              esp_event_base_t base,
                              int32_t event_id,
                              void* event_data);

    void _handleEvent(esp_mqtt_event_handle_t event);
    void _checkRetries();
};

#endif


