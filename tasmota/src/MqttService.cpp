#include "MqttService.h"

MqttService* MqttService::instance_ = nullptr;

MqttService::MqttService() : client_(wifiClient_) { instance_ = this; }

void MqttService::begin(const char* host, uint16_t port, const char* user, const char* password) {
  host_ = host ? host : "";
  port_ = port;
  user_ = user ? user : "";
  password_ = password ? password : "";
  client_.setServer(host_.c_str(), port_);
  client_.setKeepAlive(60);
  client_.setSocketTimeout(30);
  client_.setBufferSize(512);
  client_.setCallback(rawCallback);
}

void MqttService::tick(unsigned long retryDelayMs) {
  if (!client_.connected()) {
    unsigned long now = millis();
    if (now - lastAttemptMs_ >= retryDelayMs) {
      lastAttemptMs_ = now;
      reconnect();
    }
    return;
  }

  client_.loop();
}

bool MqttService::connected() { return client_.connected(); }

bool MqttService::publish(const String& topic, const String& payload, bool retained) {
  if (!client_.connected()) {
    return false;
  }
  return client_.publish(topic.c_str(), payload.c_str(), retained);
}

bool MqttService::subscribe(const String& topic) {
  for (const auto& existing : subscriptions_) {
    if (existing == topic) {
      return client_.connected() ? client_.subscribe(topic.c_str()) : true;
    }
  }

  subscriptions_.push_back(topic);
  return client_.connected() ? client_.subscribe(topic.c_str()) : true;
}

void MqttService::setCallback(MessageCallback callback) {
  callback_ = std::move(callback);
}

void MqttService::rawCallback(char* topic, byte* payload, unsigned int length) {
  if (!instance_ || !instance_->callback_) {
    return;
  }

  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; ++i) {
    message += static_cast<char>(payload[i]);
  }

  instance_->callback_(String(topic), message);
}

bool MqttService::reconnect() {
  String clientId = makeClientId();
  bool ok = false;

  if (user_.length() || password_.length()) {
    ok = client_.connect(clientId.c_str(), user_.c_str(), password_.c_str());
  } else {
    ok = client_.connect(clientId.c_str());
  }

  if (!ok) {
    return false;
  }

  for (const auto& topic : subscriptions_) {
    client_.subscribe(topic.c_str());
  }
  return true;
}

String MqttService::makeClientId() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "esp8285_%06X_%lu", ESP.getChipId(),
           static_cast<unsigned long>(millis()));
  return String(buffer);
}
