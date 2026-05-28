#include "EspNowLink.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include "../../common/ExperimentConfig.h"

namespace {
constexpr uint8_t PACKET_VERSION = 1;
constexpr uint8_t MAX_PENDING_EVENTS = 8;
constexpr uint8_t MAX_RETRY_COUNT = 10;
constexpr uint32_t RETRY_INTERVAL_MS = 100;
constexpr uint8_t MAX_WIFI_CHANNEL = 13;
constexpr uint32_t SLAVE_STATE_LOG_INTERVAL_MS = 10000;

uint8_t MASTER_MAC[6] = {0x68, 0x67, 0x25, 0xEC, 0xA5, 0x84};

enum ChannelState : uint8_t {
  SEARCHING,
  SYNCED,
};

struct PendingEvent {
  bool active = false;
  EspNowPacket packet = {};
  uint32_t lastSendMs = 0;
};

uint8_t g_slaveId = 0;
uint32_t g_bootId = 0;
uint32_t g_nextSeq = 1;
PendingEvent g_pendingEvents[MAX_PENDING_EVENTS] = {};
uint16_t g_nfcPollIntervalMs = 0;
uint16_t g_removeTimeMs = 0;
uint32_t g_lastStateLogMs = 0;

volatile uint8_t g_currentChannel = 1;
volatile ChannelState g_channelState = SEARCHING;
volatile uint32_t g_lastHeartbeatMs = 0;
volatile uint32_t g_lastScanMs = 0;
uint32_t g_lastHeartbeatLogMs = 0;

const char* packetAction(uint8_t type) {
  return type == PACKET_NFC_READ ? "READ" : "REMOVE";
}

bool isMasterMacConfigured() {
  for (uint8_t value : MASTER_MAC) {
    if (value != 0x00) {
      return true;
    }
  }
  return false;
}

void syncMasterPeerChannel(uint8_t channel, const char* reason) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, MASTER_MAC, sizeof(MASTER_MAC));
  peerInfo.channel = channel;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  esp_err_t result = ESP_OK;
  if (esp_now_is_peer_exist(MASTER_MAC)) {
    result = esp_now_mod_peer(&peerInfo);
    Serial.printf("[Slave-%u][Peer] mod_master channel=%u reason=%s result=%s\n",
                  g_slaveId,
                  channel,
                  reason,
                  esp_err_to_name(result));
    return;
  }

  result = esp_now_add_peer(&peerInfo);
  Serial.printf("[Slave-%u][Peer] add_master channel=%u reason=%s result=%s\n",
                g_slaveId,
                channel,
                reason,
                esp_err_to_name(result));
}

PendingEvent* allocatePendingEvent() {
  for (PendingEvent& event : g_pendingEvents) {
    if (!event.active) {
      return &event;
    }
  }
  return nullptr;
}

PendingEvent* findLatestPendingEvent(uint8_t nfcIndex) {
  PendingEvent* latest = nullptr;
  for (PendingEvent& event : g_pendingEvents) {
    if (!event.active || event.packet.nfcIndex != nfcIndex) {
      continue;
    }
    if (!latest || event.packet.seq > latest->packet.seq) {
      latest = &event;
    }
  }
  return latest;
}

PendingEvent* findPendingEvent(uint32_t seq, uint32_t bootId, uint8_t type, uint8_t nfcIndex) {
  for (PendingEvent& event : g_pendingEvents) {
    if (!event.active) {
      continue;
    }
    if (event.packet.seq == seq &&
        event.packet.bootId == bootId &&
        event.packet.type == type &&
        event.packet.nfcIndex == nfcIndex) {
      return &event;
    }
  }
  return nullptr;
}

PendingEvent* firstPollReadyEvent() {
  PendingEvent* selected = nullptr;
  for (PendingEvent& event : g_pendingEvents) {
    if (!event.active) {
      continue;
    }
    if (event.packet.pollSeq == 0) {
      return &event;
    }
    if (!selected || event.packet.seq < selected->packet.seq) {
      selected = &event;
    }
  }
  return selected;
}

uint8_t pendingEventCount() {
  uint8_t count = 0;
  for (const PendingEvent& event : g_pendingEvents) {
    if (event.active) {
      count++;
    }
  }
  return count;
}

void transmitPendingEvent(PendingEvent& event, bool retry) {
  event.packet.attempt++;
  event.lastSendMs = millis();
  const esp_err_t err = esp_now_send(
      MASTER_MAC,
      reinterpret_cast<const uint8_t*>(&event.packet),
      sizeof(event.packet));

  if (!retry) {
    Serial.printf("[Slave-%u][NFC-%u %s] queued_for_poll seq=%lu send=%s\n",
                  g_slaveId,
                  event.packet.nfcIndex,
                  packetAction(event.packet.type),
                  static_cast<unsigned long>(event.packet.seq),
                  esp_err_to_name(err));
  } else {
    Serial.printf("[Slave-%u][Retry] NFC-%u %s seq=%lu attempt=%u send=%s\n",
                  g_slaveId,
                  event.packet.nfcIndex,
                  packetAction(event.packet.type),
                  static_cast<unsigned long>(event.packet.seq),
                  event.packet.attempt,
                  esp_err_to_name(err));
  }
}
}  // namespace

void EspNowLink::begin(uint8_t slaveId, uint32_t bootId, uint8_t initialChannel) {
  g_slaveId = slaveId;
  g_bootId = bootId;
  g_currentChannel = initialChannel;

  WiFi.disconnect(false, true);
  delay(100);

  esp_wifi_start();
  esp_wifi_set_channel(g_currentChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_ps(WIFI_PS_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.printf("[Slave-%u][ERROR] esp_now_init failed\n", g_slaveId);
    return;
  }

  esp_now_register_send_cb(EspNowLink::onDataSentThunk);
  esp_now_register_recv_cb(EspNowLink::onDataRecvThunk);
  syncMasterPeerChannel(g_currentChannel, "begin");
}

void EspNowLink::loop() {
  processPendingRetries();
  processChannelScan();

  const uint32_t nowMs = millis();
  if (g_channelState == SYNCED) {
    static uint32_t lastTxHeartbeatMs = 0;
    if (nowMs - lastTxHeartbeatMs >= 11000) {
      lastTxHeartbeatMs = nowMs;

      EspNowPacket packet = {};
      packet.version = PACKET_VERSION;
      packet.type = PACKET_HEARTBEAT;
      packet.slaveId = g_slaveId;
      packet.bootId = g_bootId;
      packet.seq = g_nextSeq++;
      packet.eventTimeMs = nowMs;
      esp_err_t hbResult =
          esp_now_send(MASTER_MAC, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
      Serial.printf("[Slave-%u][State] link=SYNCED channel=%u pending=%u hb_send=%s\n",
                    g_slaveId,
                    g_currentChannel,
                    pendingEventCount(),
                    esp_err_to_name(hbResult));
    }
  } else if (nowMs - g_lastStateLogMs >= SLAVE_STATE_LOG_INTERVAL_MS) {
    g_lastStateLogMs = nowMs;
    Serial.printf("[Slave-%u][State] link=SEARCHING channel=%u pending=%u\n",
                  g_slaveId,
                  g_currentChannel,
                  pendingEventCount());
  }
}

void EspNowLink::queueNfcPacket(uint8_t nfcIndex, PacketType type, const String& uid) {
  if (!isMasterMacConfigured()) {
    return;
  }

  PendingEvent* event = findLatestPendingEvent(nfcIndex);
  if (event) {
    const bool sameUid = strncmp(event->packet.uid, uid.c_str(), sizeof(event->packet.uid)) == 0;
    if (event->packet.pollSeq == 0 && sameUid) {
      if (event->packet.type == type) {
        event->packet.eventTimeMs = millis();
        Serial.printf("[Slave-%u][Coalesce] NFC-%u keep %s seq=%lu uid=%s\n",
                      g_slaveId,
                      nfcIndex,
                      packetAction(type),
                      static_cast<unsigned long>(event->packet.seq),
                      event->packet.uid);
        return;
      }

      event->packet.type = type;
      event->packet.eventTimeMs = millis();
      event->packet.attempt = 0;
      event->packet.pollSeq = 0;
      event->packet.pollSentMs = 0;
      event->packet.masterRxMs = 0;
      event->packet.removeTimeMs = g_removeTimeMs;
      event->packet.nfcPollIntervalMs = g_nfcPollIntervalMs;
      Serial.printf("[Slave-%u][Coalesce] NFC-%u flip_to=%s seq=%lu uid=%s\n",
                    g_slaveId,
                    nfcIndex,
                    packetAction(type),
                    static_cast<unsigned long>(event->packet.seq),
                    event->packet.uid);
      return;
    }
  }

  event = allocatePendingEvent();
  if (!event) {
    Serial.printf("[Slave-%u][QueueFull] NFC-%u %s uid=%s\n",
                  g_slaveId,
                  nfcIndex,
                  packetAction(type),
                  uid.c_str());
    return;
  }

  event->active = true;
  event->lastSendMs = 0;
  event->packet = {};
  event->packet.version = PACKET_VERSION;
  event->packet.type = type;
  event->packet.slaveId = g_slaveId;
  event->packet.nfcIndex = nfcIndex;
  event->packet.bootId = g_bootId;
  event->packet.seq = g_nextSeq++;
  event->packet.eventTimeMs = millis();
  event->packet.removeTimeMs = g_removeTimeMs;
  event->packet.nfcPollIntervalMs = g_nfcPollIntervalMs;
  uid.toCharArray(event->packet.uid, sizeof(event->packet.uid));

  Serial.printf("[Slave-%u][Queued] NFC-%u %s seq=%lu uid=%s\n",
                g_slaveId,
                nfcIndex,
                packetAction(type),
                static_cast<unsigned long>(event->packet.seq),
                event->packet.uid);
}

void EspNowLink::setTimingSnapshot(uint16_t nfcPollIntervalMs, uint16_t removeTimeMs) {
  g_nfcPollIntervalMs = nfcPollIntervalMs;
  g_removeTimeMs = removeTimeMs;
}

void EspNowLink::sendSlaveBoot() {
  EspNowPacket packet = {};
  packet.version = PACKET_VERSION;
  packet.type = PACKET_SLAVE_BOOT;
  packet.slaveId = g_slaveId;
  packet.bootId = g_bootId;
  packet.seq = g_nextSeq++;
  packet.eventTimeMs = millis();

  esp_err_t err = esp_now_send(MASTER_MAC, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  Serial.printf("[Slave-%u][Boot] sync_complete result=%s\n",
                g_slaveId,
                esp_err_to_name(err));
}

void EspNowLink::handlePoll(const EspNowPacket& pollPacket) {
  PendingEvent* event = firstPollReadyEvent();
  if (!event) {
    return;
  }

  if (event->packet.pollSeq == 0) {
    event->packet.pollSeq = pollPacket.pollSeq;
    event->packet.pollSentMs = pollPacket.pollSentMs;
  }
  transmitPendingEvent(*event, false);
}

void EspNowLink::onDataSentThunk(const uint8_t* macAddr, esp_now_send_status_t status) {
  EspNowLink::getInstance().onDataSent(macAddr, status);
}

void EspNowLink::onDataRecvThunk(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
  EspNowLink::getInstance().onDataRecv(macAddr, incomingData, len);
}

void EspNowLink::onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  (void)macAddr;
  (void)status;
}

void EspNowLink::onDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
  (void)macAddr;

  const bool isHeartbeat = (len > 0 && incomingData[0] == PACKET_HEARTBEAT);
  if (isHeartbeat) {
    MasterHeartbeat hb = {};
    const int copyLen = len < static_cast<int>(sizeof(MasterHeartbeat)) ? len : static_cast<int>(sizeof(MasterHeartbeat));
    memcpy(&hb, incomingData, copyLen);

    const bool wasSearching = (g_channelState == SEARCHING);
    g_lastHeartbeatMs = millis();
    if (wasSearching) {
      Serial.printf("[Slave-%u][Heartbeat] master_found channel=%u\n", g_slaveId, hb.channel);
      g_channelState = SYNCED;
      g_lastScanMs = g_lastHeartbeatMs;
    }

    if (g_currentChannel != hb.channel) {
      g_currentChannel = hb.channel;
      esp_wifi_set_channel(g_currentChannel, WIFI_SECOND_CHAN_NONE);
      syncMasterPeerChannel(g_currentChannel, "heartbeat_channel_update");
    }
    if (millis() - g_lastHeartbeatLogMs >= 5000) {
      g_lastHeartbeatLogMs = millis();
      Serial.printf("[Slave-%u][Heartbeat] synced channel=%u seq=%lu\n",
                    g_slaveId,
                    g_currentChannel,
                    static_cast<unsigned long>(hb.seq));
    }
    if (hb.seq <= 1 || wasSearching) {
      syncMasterPeerChannel(g_currentChannel, "heartbeat_initial");
    }
    if (wasSearching) {
      sendSlaveBoot();
    }
    return;
  }

  if (len != static_cast<int>(sizeof(EspNowPacket))) {
    return;
  }

  EspNowPacket packet = {};
  memcpy(&packet, incomingData, sizeof(packet));

  if (packet.version != PACKET_VERSION) {
    return;
  }

  if (packet.type == PACKET_MASTER_POLL) {
    handlePoll(packet);
    return;
  }

  if (packet.type != PACKET_MASTER_ACK) {
    return;
  }

  PendingEvent* pending = findPendingEvent(packet.seq, packet.bootId, packet.ackedType, packet.nfcIndex);
  if (!pending) {
    return;
  }

  Serial.printf("[Slave-%u][Ack] NFC-%u %s seq=%lu attempt=%u\n",
                g_slaveId,
                packet.nfcIndex,
                packetAction(pending->packet.type),
                static_cast<unsigned long>(packet.seq),
                pending->packet.attempt);
  pending->active = false;
}

void EspNowLink::processPendingRetries() {
  const uint32_t nowMs = millis();
  for (PendingEvent& event : g_pendingEvents) {
    if (!event.active || event.packet.pollSeq == 0) {
      continue;
    }
    if (event.packet.attempt >= MAX_RETRY_COUNT) {
      Serial.printf("[Slave-%u][Drop] NFC-%u %s seq=%lu no_ack\n",
                    g_slaveId,
                    event.packet.nfcIndex,
                    packetAction(event.packet.type),
                    static_cast<unsigned long>(event.packet.seq));
      event.active = false;
      continue;
    }
    if (nowMs - event.lastSendMs >= RETRY_INTERVAL_MS) {
      transmitPendingEvent(event, true);
    }
  }
}

void EspNowLink::processChannelScan() {
  const uint32_t nowMs = millis();
  const uint32_t elapsedHb = nowMs - g_lastHeartbeatMs;
  if (g_channelState == SYNCED &&
      elapsedHb > ExperimentConfig::kHeartbeatTimeoutMs &&
      elapsedHb < 0x7FFFFFFF) {
    Serial.printf("[Slave-%u][Channel] heartbeat_lost, scanning\n", g_slaveId);
    g_channelState = SEARCHING;
  }

  const uint32_t elapsedScan = nowMs - g_lastScanMs;
  if (g_channelState == SEARCHING &&
      elapsedScan > ExperimentConfig::kChannelScanIntervalMs &&
      elapsedScan < 0x7FFFFFFF) {
    g_lastScanMs = nowMs;
    g_currentChannel = (g_currentChannel % MAX_WIFI_CHANNEL) + 1;
    Serial.printf("[Slave-%u][Channel] scanning channel=%u\n", g_slaveId, g_currentChannel);
    esp_wifi_set_channel(g_currentChannel, WIFI_SECOND_CHAN_NONE);
  }
}
