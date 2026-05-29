/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - 경량 MQTT publish 래퍼 타입/계약 정의.
*/
#ifndef MQTT_LITE_H
#define MQTT_LITE_H

#include <Arduino.h>
#include <UIPEthernet.h>
#include <vector>
#include <map>
#include <functional>
#include <ArduinoJson.h>

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
  - retry/timeoutMs로 실패 시 재시도 전략을 정의한다.
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
    Mqtt(MqttConfig cfg);

    void begin();
    void connect();
    void disconnect();
    bool isConnected() const;
    void setAutoReconnect(bool value);

    void onConnect(std::function<void(bool)> cb);
    void onDisconnect(std::function<void()> cb);
    void onReceived(MqttSubscribeConfig cfg);
    void publish(MqttPublishConfig cfg);

private:
    MqttConfig _config;
    EthernetClient _client;
    String _clientId;

    bool _connected = false;
    bool _hasEverConnected = false;

    std::vector<MqttSubscribeConfig> _subs;

    struct PendingMessage {
        String topic;
        String payload;
        int qos;
        bool retain;

        int retryCount = 0;
        int maxRetry = 0;
        uint32_t baseTimeout = 0;
        uint32_t nextRetryAt = 0;

        std::function<void()> onAck;
        std::function<void()> onFail;
    };

    std::map<int, PendingMessage> _pending;

    std::function<void(bool)> _onConnect;
    std::function<void()> _onDisconnect;

    bool _openSocket();
    bool _sendConnectPacket();
    bool _writePacket(const uint8_t* data, size_t len);
    bool _writeStringField(const String& value);
    bool _publishInternal(const MqttPublishConfig& cfg);
    size_t _encodeRemainingLength(uint32_t value, uint8_t* out);
    void _markDisconnected();
    void _checkRetries();
};

#endif
