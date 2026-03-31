#pragma once

namespace Config {
constexpr uint8_t kButtonPin = 3;
constexpr uint8_t kRelayPin = 12;
constexpr uint8_t kLedPin = 13;
constexpr uint8_t kAuxLedPin = 14;

constexpr unsigned long kBootBlinkMs = 3000;
constexpr unsigned long kInitialWifiWaitMs = 15000;
constexpr unsigned long kStatusPublishIntervalMs = 5000;
constexpr unsigned long kWifiRetryDelayMs = 500;
constexpr unsigned long kMqttRetryDelayMs = 3000;
constexpr unsigned long kDebounceMs = 40;
}  // namespace Config
