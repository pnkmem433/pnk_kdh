/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - MQTT 이벤트 핸들러와 QoS1 재시도 큐 구현.
*/
#include "MqttLite.h"

/*
  Mqtt(cfg)
  - 입력: MQTT 접속/인증/재연결 정책
  - 부작용: 설정값 저장
*/
Mqtt::Mqtt(MqttConfig cfg)
: _config(cfg)
{
}

/*
  begin()
  - ESP-IDF mqtt client 구성/초기화만 수행한다.
  - 실제 네트워크 세션 시작은 connect()에서 수행.
*/
void Mqtt::begin()
{
    esp_mqtt_client_config_t mqtt_cfg = {};

    String uri = _config.useTls
        ? "mqtts://" + _config.host
        : "mqtt://" + _config.host;

    mqtt_cfg.broker.address.uri = uri.c_str();
    mqtt_cfg.broker.address.port = _config.port;

    if (_config.user.length())
        mqtt_cfg.credentials.username = _config.user.c_str();

    if (_config.password.length())
        mqtt_cfg.credentials.authentication.password = _config.password.c_str();

    if (_config.useTls && _config.caCert.length())
        mqtt_cfg.broker.verification.certificate = _config.caCert.c_str();

    mqtt_cfg.network.disable_auto_reconnect = !_config.autoReconnect;

    _client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(
        _client,
        MQTT_EVENT_ANY,
        _eventHandler,
        this
    );
}

void Mqtt::connect()
{
    // begin() 완료 후 실제 MQTT 세션 시작
    if (_client)
        esp_mqtt_client_start(_client);
}

void Mqtt::disconnect()
{
    // 현재 세션 중지(자동 재연결 정책과 별개)
    if (_client)
        esp_mqtt_client_stop(_client);
}

bool Mqtt::isConnected() const
{
    return _connected;
}

/*
  setAutoReconnect(value)
  - 내부 정책값 변경. 필요 시 begin/connect 사이클 재구성이 필요할 수 있다.
*/
void Mqtt::setAutoReconnect(bool value)
{
    _config.autoReconnect = value;
}

void Mqtt::onConnect(std::function<void(bool)> cb)
{
    _onConnect = cb;
}

void Mqtt::onDisconnect(std::function<void()> cb)
{
    _onDisconnect = cb;
}

/*
  onReceived(cfg)
  - 구독 정의를 내부 목록에 추가하고, 연결 중이면 즉시 subscribe한다.
*/
void Mqtt::onReceived(MqttSubscribeConfig cfg)
{
    _subs.push_back(cfg);

    if (_connected)
        esp_mqtt_client_subscribe(_client, cfg.topic.c_str(), cfg.qos);
}

/*
  publish()
  - QoS1일 때만 pending map에 넣어 ACK 타임아웃 재시도 대상으로 관리한다.
*/
void Mqtt::publish(MqttPublishConfig cfg)
{
    if (!_connected) return;

    int msg_id = esp_mqtt_client_publish(
        _client,
        cfg.topic.c_str(),
        cfg.payload.c_str(),
        0,
        cfg.qos,
        cfg.retain
    );

    if (cfg.qos == 1 && msg_id > 0)
    {
        PendingMessage p;
        p.topic = cfg.topic;
        p.payload = cfg.payload;
        p.qos = cfg.qos;
        p.retain = cfg.retain;

        p.maxRetry = cfg.retry;
        p.baseTimeout = cfg.timeoutMs;
        p.nextRetryAt = millis() + cfg.timeoutMs;

        p.onAck = cfg.onAck;
        p.onFail = cfg.onFail;

        _pending[msg_id] = p;
    }
}

void Mqtt::_eventHandler(void* handler_args,
                         esp_event_base_t base,
                         int32_t event_id,
                         void* event_data)
{
    // C 스타일 콜백을 인스턴스 메서드로 브리지
    Mqtt* self = static_cast<Mqtt*>(handler_args);
    self->_handleEvent((esp_mqtt_event_handle_t)event_data);
}

/*
  _handleEvent(event)
  - MQTT 이벤트별 상태 전이/콜백/재시도 체크를 수행한다.
  - 주의: DATA 이벤트에서 콜백은 등록 순서대로 호출된다.
*/
void Mqtt::_handleEvent(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        _connected = true;

        for (auto& s : _subs)
            esp_mqtt_client_subscribe(_client, s.topic.c_str(), s.qos);

        bool isReconnect = _hasEverConnected;
        _hasEverConnected = true;

        if (_onConnect)
            _onConnect(isReconnect);
    }
    break;

    case MQTT_EVENT_DISCONNECTED:
    {
        _connected = false;

        if (_onDisconnect)
            _onDisconnect();
    }
    break;

    case MQTT_EVENT_DATA:
    {
        MqttMessage msg;

        msg.topic = String(event->topic, event->topic_len);
        msg.raw = String(event->data, event->data_len);
        msg.qos = event->qos;
        msg.retain = event->retain;
        msg.messageId = event->msg_id;

        DeserializationError err =
            deserializeJson(msg.doc, msg.raw);

        if (!err)
        {
            msg.isJson = true;
            msg.json = msg.doc.as<JsonVariant>();
        }

        // 현재 구현은 topic 필터 없이 등록된 콜백 전체 호출.
        // topic별 분기는 상위 콜백에서 수행한다.
        for (auto& s : _subs)
        {
            if (s.callback)
                s.callback(msg);
        }
    }
    break;

    case MQTT_EVENT_PUBLISHED:
    {
        int id = event->msg_id;

        auto it = _pending.find(id);
        if (it != _pending.end())
        {
            if (it->second.onAck)
                it->second.onAck();

            _pending.erase(it);
        }
    }
    break;

    default:
        break;
    }

    _checkRetries();
}

/*
  _checkRetries()
  - 재시도 정책:
    - retryCount 증가마다 baseTimeout * 2^n 상한 계산
    - 최대 60초로 캡
    - random(0, maxDelay) 지터 적용으로 다중 장치 동시 재시도 폭주 완화
*/
void Mqtt::_checkRetries()
{
    uint32_t now = millis();

    for (auto it = _pending.begin(); it != _pending.end(); )
    {
        auto& p = it->second;

        if (now >= p.nextRetryAt)
        {
            if (p.retryCount >= p.maxRetry)
            {
                if (p.onFail)
                    p.onFail();

                it = _pending.erase(it);
                continue;
            }

            p.retryCount++;

            uint32_t maxDelay = p.baseTimeout * (1 << p.retryCount);
            if (maxDelay > 60000) maxDelay = 60000;

            uint32_t delay = random(0, maxDelay);
            p.nextRetryAt = now + delay;

            esp_mqtt_client_publish(
                _client,
                p.topic.c_str(),
                p.payload.c_str(),
                0,
                p.qos,
                p.retain
            );
        }

        ++it;
    }
}


