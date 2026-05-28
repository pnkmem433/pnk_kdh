/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - 경량 MQTT 3.1.1 publish 클라이언트와 재시도 큐 구현.
*/
#include "MqttLite.h"

Mqtt::Mqtt(MqttConfig cfg)
: _config(cfg)
{
    uint64_t chipId = ESP.getEfuseMac();
    _clientId = "ccretail-" + String(
        static_cast<uint32_t>(chipId & 0xFFFFFFFFULL), HEX
    );
}

void Mqtt::begin()
{
    _client.setTimeout(2);
}

void Mqtt::connect()
{
    bool wasConnected = _connected;

    if (!_openSocket() || !_sendConnectPacket())
    {
        if (wasConnected)
            _markDisconnected();
        return;
    }

    _connected = true;

    bool isReconnect = _hasEverConnected;
    _hasEverConnected = true;

    if (_onConnect)
        _onConnect(isReconnect);
}

void Mqtt::disconnect()
{
    if (_client.connected())
    {
        uint8_t packet[] = {0xE0, 0x00};
        _client.write(packet, sizeof(packet));
    }

    _client.stop();
    _markDisconnected();
}

bool Mqtt::isConnected() const
{
    return _connected;
}

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

void Mqtt::onReceived(MqttSubscribeConfig cfg)
{
    _subs.push_back(cfg);
}

void Mqtt::publish(MqttPublishConfig cfg)
{
    if (!isConnected() && _config.autoReconnect)
        connect();

    if (!isConnected())
    {
        if (cfg.onFail)
            cfg.onFail();
        return;
    }

    if (_publishInternal(cfg))
    {
        if (cfg.onAck)
            cfg.onAck();
    }
    else
    {
        _markDisconnected();

        if (cfg.qos > 0 && cfg.retry > 0)
        {
            static int nextMessageId = 1;

            PendingMessage pending;
            pending.topic = cfg.topic;
            pending.payload = cfg.payload;
            pending.qos = cfg.qos;
            pending.retain = cfg.retain;
            pending.maxRetry = cfg.retry;
            pending.baseTimeout = cfg.timeoutMs;
            pending.nextRetryAt = millis() + cfg.timeoutMs;
            pending.onAck = cfg.onAck;
            pending.onFail = cfg.onFail;

            _pending[nextMessageId++] = pending;
        }
        else if (cfg.onFail)
        {
            cfg.onFail();
        }
    }

    _checkRetries();
}

bool Mqtt::_openSocket()
{
    if (_config.useTls)
        return false;

    if (_client.connected())
        return true;

    _client.stop();
    return _client.connect(_config.host.c_str(), _config.port);
}

bool Mqtt::_sendConnectPacket()
{
    uint8_t flags = 0x02;
    const uint16_t keepAliveSec = 30;

    if (_config.user.length()) flags |= 0x80;
    if (_config.password.length()) flags |= 0x40;

    uint32_t remainingLength = 10 + 2 + _clientId.length();

    if (_config.user.length())
        remainingLength += 2 + _config.user.length();

    if (_config.password.length())
        remainingLength += 2 + _config.password.length();

    uint8_t header[5] = {0x10};
    size_t headerLen =
        1 + _encodeRemainingLength(remainingLength, &header[1]);

    uint8_t variableHeader[] = {
        0x00, 0x04, 'M', 'Q', 'T', 'T',
        0x04,
        flags,
        static_cast<uint8_t>(keepAliveSec >> 8),
        static_cast<uint8_t>(keepAliveSec & 0xFF)
    };

    if (!_writePacket(header, headerLen) ||
        !_writePacket(variableHeader, sizeof(variableHeader)) ||
        !_writeStringField(_clientId))
        return false;

    if (_config.user.length() && !_writeStringField(_config.user))
        return false;

    if (_config.password.length() && !_writeStringField(_config.password))
        return false;

    unsigned long started = millis();
    while (_client.available() < 4 && millis() - started < 2000)
        delay(5);

    if (_client.available() < 4)
        return false;

    uint8_t ack[4];
    if (_client.read(ack, sizeof(ack)) != sizeof(ack))
        return false;

    return ack[0] == 0x20 && ack[1] == 0x02 && ack[3] == 0x00;
}

bool Mqtt::_writePacket(const uint8_t* data, size_t len)
{
    return _client.write(data, len) == len;
}

bool Mqtt::_writeStringField(const String& value)
{
    uint8_t lenBuf[] = {
        static_cast<uint8_t>(value.length() >> 8),
        static_cast<uint8_t>(value.length() & 0xFF)
    };

    if (!_writePacket(lenBuf, sizeof(lenBuf)))
        return false;

    return _client.write(
               reinterpret_cast<const uint8_t*>(value.c_str()),
               value.length()
           ) == value.length();
}

bool Mqtt::_publishInternal(const MqttPublishConfig& cfg)
{
    uint8_t fixedHeader = 0x30;
    if (cfg.retain) fixedHeader |= 0x01;
    if (cfg.qos == 1) fixedHeader |= 0x02;

    uint32_t remainingLength = 2 + cfg.topic.length() + cfg.payload.length();
    uint8_t encodedLength[4];
    size_t encodedLengthSize =
        _encodeRemainingLength(remainingLength, encodedLength);

    if (!_writePacket(&fixedHeader, 1) ||
        !_writePacket(encodedLength, encodedLengthSize) ||
        !_writeStringField(cfg.topic))
        return false;

    return _client.write(
               reinterpret_cast<const uint8_t*>(cfg.payload.c_str()),
               cfg.payload.length()
           ) == cfg.payload.length();
}

size_t Mqtt::_encodeRemainingLength(uint32_t value, uint8_t* out)
{
    size_t i = 0;

    do
    {
        uint8_t byte = value % 128;
        value /= 128;

        if (value > 0)
            byte |= 0x80;

        out[i++] = byte;
    }
    while (value > 0 && i < 4);

    return i;
}

void Mqtt::_markDisconnected()
{
    bool wasConnected = _connected;
    _connected = false;
    _client.stop();

    if (wasConnected && _onDisconnect)
        _onDisconnect();
}

void Mqtt::_checkRetries()
{
    uint32_t now = millis();

    for (auto it = _pending.begin(); it != _pending.end(); )
    {
        auto& p = it->second;

        if (now < p.nextRetryAt)
        {
            ++it;
            continue;
        }

        if (p.retryCount >= p.maxRetry)
        {
            if (p.onFail)
                p.onFail();

            it = _pending.erase(it);
            continue;
        }

        p.retryCount++;

        if (!isConnected() && _config.autoReconnect)
            connect();

        if (!isConnected())
        {
            p.nextRetryAt = now + p.baseTimeout;
            ++it;
            continue;
        }

        MqttPublishConfig retryCfg = {
            p.topic,
            p.payload,
            p.qos,
            p.retain,
            p.maxRetry,
            p.baseTimeout,
            p.onAck,
            p.onFail
        };

        if (_publishInternal(retryCfg))
        {
            if (p.onAck)
                p.onAck();

            it = _pending.erase(it);
            continue;
        }

        p.nextRetryAt = now + p.baseTimeout;
        ++it;
    }
}
