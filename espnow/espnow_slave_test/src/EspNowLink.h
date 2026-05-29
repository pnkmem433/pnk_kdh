#pragma once

#include <Arduino.h>
#include <esp_now.h>

enum PacketType : uint8_t {
  PACKET_NFC_READ = 1,
  PACKET_NFC_REMOVE = 2,
  PACKET_MASTER_ACK = 3,
  PACKET_SLAVE_BOOT = 4,
  PACKET_HEARTBEAT = 5,
  PACKET_MASTER_POLL = 6,
};

struct EspNowPacket {
  uint8_t version;
  uint8_t type;
  uint8_t slaveId;
  uint8_t nfcIndex;
  uint8_t attempt;
  uint8_t ackedType;
  uint16_t removeTimeMs;
  uint32_t bootId;
  uint32_t seq;
  uint32_t eventTimeMs;
  uint32_t pollSeq;
  uint32_t pollSentMs;
  uint32_t masterRxMs;
  uint16_t nfcPollIntervalMs;
  uint16_t reserved;
  char uid[32];
};

struct MasterHeartbeat {
  uint8_t type;
  uint8_t channel;
  uint16_t reserved;
  uint32_t seq;
  uint32_t uptimeSec;
};

class EspNowLink {
public:
  static EspNowLink& getInstance() {
    static EspNowLink instance;
    return instance;
  }

  void begin(uint8_t slaveId, uint32_t bootId, uint8_t initialChannel = 1);
  void loop();
  void queueNfcPacket(uint8_t nfcIndex, PacketType type, const String& uid);
  void setTimingSnapshot(uint16_t nfcPollIntervalMs, uint16_t removeTimeMs);

private:
  EspNowLink() = default;

  void sendSlaveBoot();
  void processPendingRetries();
  void processChannelScan();
  void handlePoll(const EspNowPacket& pollPacket);

  static void onDataSentThunk(const uint8_t* macAddr, esp_now_send_status_t status);
  static void onDataRecvThunk(const uint8_t* macAddr, const uint8_t* incomingData, int len);
  void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status);
  void onDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len);
};
