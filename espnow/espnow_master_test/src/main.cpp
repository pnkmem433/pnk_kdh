#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <PubSubClient.h>
#include "../../common/ExperimentConfig.h"

namespace {

constexpr uint32_t BAUD = 115200;
constexpr uint8_t PACKET_VERSION = 1;
constexpr size_t UID_MAX_LEN = 32;
constexpr size_t DEDUP_CACHE_SIZE = 24;
constexpr size_t MAX_KNOWN_SLAVES = 4;
constexpr uint32_t SLAVE_TIMEOUT_MS = 30000;
constexpr uint32_t POLL_TX_TIMEOUT_MS = 40;
constexpr uint32_t POLL_NO_MEM_BACKOFF_MS = 25;
constexpr uint32_t SEND_FAIL_LOG_INTERVAL_MS = 5000;
constexpr uint32_t STATE_LOG_INTERVAL_MS = 10000;

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
  char uid[UID_MAX_LEN];
};

struct MasterHeartbeat {
  uint8_t type;
  uint8_t channel;
  uint16_t reserved;
  uint32_t seq;
  uint32_t uptimeSec;
};

struct SeenPacket {
  bool used;
  uint8_t slaveId;
  uint8_t type;
  uint32_t bootId;
  uint32_t seq;
};

struct KnownSlave {
  bool inUse = false;
  uint8_t slotId = 0;
  uint8_t advertisedSlaveId = 0;
  uint8_t mac[ESP_NOW_ETH_ALEN] = {};
  uint32_t lastSeenMs = 0;
  uint32_t lastHeartbeatMs = 0;
  bool isConsideredLost = false;
  bool onlineAnnounced = false;
};

struct CsvLogRow {
  uint32_t testId;
  char phase[24];
  char environment[16];
  char pattern[24];
  uint8_t slaveId;
  uint8_t nfcIndex;
  char uid[UID_MAX_LEN];
  char eventName[16];
  uint32_t bootId;
  uint32_t packetSeq;
  uint32_t pollSeq;
  uint8_t retryCount;
  uint16_t masterPollIntervalMs;
  uint16_t removeTimeMs;
  uint16_t nfcPollIntervalMs;
  uint32_t t0SlaveEventMs;
  uint32_t t1PollMs;
  uint32_t t2RecvMs;
  uint32_t t3MqttMs;
};

struct TestCounters {
  uint32_t totalEvents = 0;
  uint32_t pickDownEvents = 0;
  uint32_t pickUpEvents = 0;
  uint32_t publishSuccess = 0;
  uint32_t publishFail = 0;
  uint32_t goalEventCount = 0;
};

struct SlaveEventStats {
  bool used = false;
  uint8_t slaveId = 0;
  uint32_t pickDownEvents = 0;
  uint32_t pickUpEvents = 0;
  uint32_t lastEventMs = 0;
  char lastEventName[16] = {};
};

struct LogicalTagState {
  bool used = false;
  uint8_t slaveId = 0;
  uint8_t nfcIndex = 0;
  bool present = false;
  char uid[UID_MAX_LEN] = {};
};

SeenPacket g_seenPackets[DEDUP_CACHE_SIZE] = {};
size_t g_nextSeenIndex = 0;
KnownSlave g_knownSlaves[MAX_KNOWN_SLAVES] = {};
uint8_t g_nextSlotId = 1;
uint32_t g_lastHeartbeatMs = 0;
uint32_t g_lastPollMs = 0;
uint32_t g_heartbeatSeq = 0;
uint32_t g_pollSeq = 1;
size_t g_nextPollIndex = 0;
uint32_t g_nextTestId = 1;
uint32_t g_testStartMs = 0;
uint32_t g_lastTimerSummaryMs = 0;
uint32_t g_pollBackoffUntilMs = 0;
uint32_t g_pollInFlightSinceMs = 0;
uint32_t g_lastSendFailLogMs = 0;
bool g_pollInFlight = false;
uint8_t g_pollInFlightMac[ESP_NOW_ETH_ALEN] = {};
TestCounters g_testCounters = {};
SlaveEventStats g_slaveEventStats[MAX_KNOWN_SLAVES] = {};
LogicalTagState g_logicalTagStates[MAX_KNOWN_SLAVES * 2] = {};

const uint8_t BROADCAST_MAC[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const char* WIFI_SSID = "CC-Retail";
const char* WIFI_PASS = "pnks1111";
const char* MQTT_BROKER = "api.pnkslab.com";
const int MQTT_PORT = 1884;
const char* MQTT_USER = "pnks";
const char* MQTT_PASS = "pnks1111";

WiFiClient g_wifiClient;
PubSubClient g_mqttClient(g_wifiClient);
QueueHandle_t g_csvQueue = nullptr;
uint32_t g_lastMqttConnectAttemptMs = 0;

const char* csvEventName(uint8_t packetType) {
  return packetType == PACKET_NFC_READ ? "PICK-DOWN" : "PICK-UP";
}

const char* mqttEventName(uint8_t packetType) {
  return packetType == PACKET_NFC_READ ? "READ" : "REMOVE";
}

const char* eventEmojiByName(const char* eventName) {
  return strncmp(eventName, "PICK-DOWN", 16) == 0 ? "🟢" : "🔴";
}

bool isGoalEvent(const char* eventName) {
  return strncmp(eventName, ExperimentConfig::kGoalEvent, 15) == 0;
}

void printMac(const uint8_t* mac);
SlaveEventStats* findOrCreateSlaveStats(uint8_t slaveId);
void noteSlaveEvent(uint8_t slaveId, const char* eventName, uint32_t eventMs);
void printPerSlaveEventSummary();
LogicalTagState* findOrCreateLogicalTagState(uint8_t slaveId, uint8_t nfcIndex);
bool shouldAcceptLogicalEvent(uint8_t slaveId, uint8_t nfcIndex, uint8_t packetType, const char* uid);

bool isTimeBasedGoal() {
  return ExperimentConfig::kGoalDurationMs > 0;
}

void formatDuration(uint32_t ms, char* out, size_t outSize) {
  const uint32_t totalSec = ms / 1000;
  const uint32_t minutes = totalSec / 60;
  const uint32_t seconds = totalSec % 60;
  snprintf(out, outSize, "%02lu:%02lu",
           static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(seconds));
}

void printCounterSummary(const char* reason) {
  if (isTimeBasedGoal()) {
    char elapsed[16];
    char target[16];
    formatDuration(ExperimentConfig::kGoalDurationMs, target, sizeof(target));
    if (g_testStartMs == 0) {
      strncpy(elapsed, "WAIT", sizeof(elapsed) - 1);
      elapsed[sizeof(elapsed) - 1] = '\0';
    } else {
      formatDuration(millis() - g_testStartMs, elapsed, sizeof(elapsed));
    }
    Serial.printf(
        "[Master][Count] reason=%s phase=%s pattern=%s goal=%s elapsed=%s/%s total=%lu pickdown=%lu pickup=%lu success=%lu fail=%lu\n",
        reason,
        ExperimentConfig::kPhase,
        ExperimentConfig::kPattern,
        ExperimentConfig::kGoalEvent,
        elapsed,
        target,
        static_cast<unsigned long>(g_testCounters.totalEvents),
        static_cast<unsigned long>(g_testCounters.pickDownEvents),
        static_cast<unsigned long>(g_testCounters.pickUpEvents),
        static_cast<unsigned long>(g_testCounters.publishSuccess),
        static_cast<unsigned long>(g_testCounters.publishFail));
    return;
  }

  Serial.printf(
      "[Master][Count] reason=%s phase=%s pattern=%s goal=%s progress=%lu/%lu total=%lu pickdown=%lu pickup=%lu success=%lu fail=%lu\n",
      reason,
      ExperimentConfig::kPhase,
      ExperimentConfig::kPattern,
      ExperimentConfig::kGoalEvent,
      static_cast<unsigned long>(g_testCounters.goalEventCount),
      static_cast<unsigned long>(ExperimentConfig::kGoalTargetCount),
      static_cast<unsigned long>(g_testCounters.totalEvents),
      static_cast<unsigned long>(g_testCounters.pickDownEvents),
      static_cast<unsigned long>(g_testCounters.pickUpEvents),
      static_cast<unsigned long>(g_testCounters.publishSuccess),
      static_cast<unsigned long>(g_testCounters.publishFail));
}

SlaveEventStats* findOrCreateSlaveStats(uint8_t slaveId) {
  for (SlaveEventStats& stats : g_slaveEventStats) {
    if (stats.used && stats.slaveId == slaveId) {
      return &stats;
    }
  }

  for (SlaveEventStats& stats : g_slaveEventStats) {
    if (!stats.used) {
      stats.used = true;
      stats.slaveId = slaveId;
      stats.pickDownEvents = 0;
      stats.pickUpEvents = 0;
      stats.lastEventMs = 0;
      stats.lastEventName[0] = '\0';
      return &stats;
    }
  }
  return nullptr;
}

void noteSlaveEvent(uint8_t slaveId, const char* eventName, uint32_t eventMs) {
  SlaveEventStats* stats = findOrCreateSlaveStats(slaveId);
  if (!stats) {
    return;
  }

  if (strncmp(eventName, "PICK-DOWN", sizeof(stats->lastEventName)) == 0) {
    stats->pickDownEvents++;
  } else if (strncmp(eventName, "PICK-UP", sizeof(stats->lastEventName)) == 0) {
    stats->pickUpEvents++;
  }

  stats->lastEventMs = eventMs;
  strncpy(stats->lastEventName, eventName, sizeof(stats->lastEventName) - 1);
  stats->lastEventName[sizeof(stats->lastEventName) - 1] = '\0';
}

void printPerSlaveEventSummary() {
  for (const SlaveEventStats& stats : g_slaveEventStats) {
    if (!stats.used) {
      continue;
    }
    Serial.printf("[Master][SlaveSummary] slave=%u pickdown=%lu pickup=%lu last=%s last_ms=%lu\n",
                  stats.slaveId,
                  static_cast<unsigned long>(stats.pickDownEvents),
                  static_cast<unsigned long>(stats.pickUpEvents),
                  stats.lastEventName[0] ? stats.lastEventName : "-",
                  static_cast<unsigned long>(stats.lastEventMs));
  }
}

LogicalTagState* findOrCreateLogicalTagState(uint8_t slaveId, uint8_t nfcIndex) {
  for (LogicalTagState& state : g_logicalTagStates) {
    if (state.used && state.slaveId == slaveId && state.nfcIndex == nfcIndex) {
      return &state;
    }
  }

  for (LogicalTagState& state : g_logicalTagStates) {
    if (!state.used) {
      state.used = true;
      state.slaveId = slaveId;
      state.nfcIndex = nfcIndex;
      state.present = false;
      state.uid[0] = '\0';
      return &state;
    }
  }
  return nullptr;
}

bool shouldAcceptLogicalEvent(uint8_t slaveId, uint8_t nfcIndex, uint8_t packetType, const char* uid) {
  LogicalTagState* state = findOrCreateLogicalTagState(slaveId, nfcIndex);
  if (!state) {
    return true;
  }

  const bool isRead = (packetType == PACKET_NFC_READ);
  const bool sameUid = strncmp(state->uid, uid, sizeof(state->uid)) == 0;

  if (isRead && state->present && sameUid) {
    return false;
  }
  if (!isRead && !state->present && sameUid) {
    return false;
  }

  state->present = isRead;
  strncpy(state->uid, uid, sizeof(state->uid) - 1);
  state->uid[sizeof(state->uid) - 1] = '\0';
  return true;
}

void printMasterStateSummary() {
  uint32_t onlineCount = 0;
  uint32_t lostCount = 0;
  for (const KnownSlave& slave : g_knownSlaves) {
    if (!slave.inUse) {
      continue;
    }
    if (slave.isConsideredLost) {
      lostCount++;
    } else {
      onlineCount++;
    }
  }

  char timerText[16];
  if (!isTimeBasedGoal()) {
    strncpy(timerText, "COUNT", sizeof(timerText) - 1);
    timerText[sizeof(timerText) - 1] = '\0';
  } else if (g_testStartMs == 0) {
    strncpy(timerText, "WAIT", sizeof(timerText) - 1);
    timerText[sizeof(timerText) - 1] = '\0';
  } else {
    formatDuration(millis() - g_testStartMs, timerText, sizeof(timerText));
  }

  Serial.printf("[Master][State] wifi_ch=%u online=%lu lost=%lu poll_inflight=%s timer=%s total=%lu pickdown=%lu pickup=%lu\n",
                WiFi.channel(),
                static_cast<unsigned long>(onlineCount),
                static_cast<unsigned long>(lostCount),
                g_pollInFlight ? "yes" : "no",
                timerText,
                static_cast<unsigned long>(g_testCounters.totalEvents),
                static_cast<unsigned long>(g_testCounters.pickDownEvents),
                static_cast<unsigned long>(g_testCounters.pickUpEvents));

  if (onlineCount == 0) {
    Serial.println("[Master][Link] waiting_for_slave");
  }
}

void printMac(const uint8_t* mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool wasSeen(const EspNowPacket& packet) {
  for (const SeenPacket& seen : g_seenPackets) {
    if (!seen.used) {
      continue;
    }
    if (seen.slaveId == packet.slaveId &&
        seen.type == packet.type &&
        seen.bootId == packet.bootId &&
        seen.seq == packet.seq) {
      return true;
    }
  }
  return false;
}

void rememberSeen(const EspNowPacket& packet) {
  SeenPacket& seen = g_seenPackets[g_nextSeenIndex];
  seen.used = true;
  seen.slaveId = packet.slaveId;
  seen.type = packet.type;
  seen.bootId = packet.bootId;
  seen.seq = packet.seq;
  g_nextSeenIndex = (g_nextSeenIndex + 1) % DEDUP_CACHE_SIZE;
}

void ensurePeerExists(const uint8_t* macAddr) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macAddr, ESP_NOW_ETH_ALEN);
  peerInfo.channel = WiFi.channel();
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  esp_err_t result = ESP_OK;
  if (esp_now_is_peer_exist(macAddr)) {
    result = esp_now_mod_peer(&peerInfo);
    if (result != ESP_OK) {
      Serial.print("[Master][Peer] mod ");
      printMac(macAddr);
      Serial.printf(" channel=%u result=%s\n", peerInfo.channel, esp_err_to_name(result));
    }
    return;
  }

  result = esp_now_add_peer(&peerInfo);
  Serial.print("[Master][Peer] add ");
  printMac(macAddr);
  Serial.printf(" channel=%u result=%s\n", peerInfo.channel, esp_err_to_name(result));
}

KnownSlave* findKnownSlaveByMac(const uint8_t* macAddr) {
  for (KnownSlave& slave : g_knownSlaves) {
    if (!slave.inUse) {
      continue;
    }
    if (memcmp(slave.mac, macAddr, ESP_NOW_ETH_ALEN) == 0) {
      return &slave;
    }
  }
  return nullptr;
}

KnownSlave* registerOrUpdateSlave(const uint8_t* macAddr, const EspNowPacket& packet) {
  KnownSlave* slave = findKnownSlaveByMac(macAddr);
  if (!slave) {
    for (KnownSlave& candidate : g_knownSlaves) {
      if (!candidate.inUse) {
        slave = &candidate;
        slave->inUse = true;
        slave->slotId = g_nextSlotId++;
        memcpy(slave->mac, macAddr, ESP_NOW_ETH_ALEN);
        break;
      }
    }
  }

  if (!slave) {
    Serial.println("[Master][WARN] known slave table full");
    return nullptr;
  }

  slave->advertisedSlaveId = packet.slaveId;
  slave->lastSeenMs = millis();
  slave->isConsideredLost = false;
  if (!slave->onlineAnnounced) {
    slave->onlineAnnounced = true;
    Serial.printf("[Master][Link] slot=%u adv_id=%u state=online mac=",
                  slave->slotId,
                  slave->advertisedSlaveId);
    printMac(slave->mac);
    Serial.println();
  }
  return slave;
}

void connectWiFi() {
  Serial.printf("[WiFi] connecting to %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  esp_wifi_set_ps(WIFI_PS_NONE);
  Serial.printf("[WiFi] connected ip=%s channel=%d\n",
                WiFi.localIP().toString().c_str(),
                WiFi.channel());
}

void connectMqtt() {
  const uint32_t now = millis();
  if (now - g_lastMqttConnectAttemptMs < 5000 && g_lastMqttConnectAttemptMs != 0) {
    return;
  }

  g_lastMqttConnectAttemptMs = now == 0 ? 1 : now;
  String clientId = "ESP32Master-" + String(random(0xffff), HEX);
  Serial.print("[MQTT] connecting...");
  if (g_mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("connected");
  } else {
    Serial.printf("failed rc=%d\n", g_mqttClient.state());
  }
}

void setupEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("[Master][ERROR] esp_now_init failed");
    return;
  }

  esp_now_register_send_cb([](const uint8_t* macAddr, esp_now_send_status_t status) {
    if (g_pollInFlight && memcmp(macAddr, g_pollInFlightMac, ESP_NOW_ETH_ALEN) == 0) {
      g_pollInFlight = false;
    }
    if (memcmp(macAddr, BROADCAST_MAC, ESP_NOW_ETH_ALEN) != 0 &&
        status != ESP_NOW_SEND_SUCCESS &&
        millis() - g_lastSendFailLogMs >= SEND_FAIL_LOG_INTERVAL_MS) {
      g_lastSendFailLogMs = millis();
      Serial.print("[Master][SendFail] ");
      printMac(macAddr);
      Serial.printf(" status=%d\n", static_cast<int>(status));
    }
  });

  esp_now_register_recv_cb([](const uint8_t* macAddr, const uint8_t* incomingData, int len) {
    if (len != static_cast<int>(sizeof(EspNowPacket))) {
      return;
    }

    EspNowPacket packet = {};
    memcpy(&packet, incomingData, sizeof(packet));
    if (packet.version != PACKET_VERSION) {
      return;
    }

    KnownSlave* slave = registerOrUpdateSlave(macAddr, packet);
    ensurePeerExists(macAddr);

    if (packet.type == PACKET_SLAVE_BOOT) {
      if (slave) {
        Serial.printf("[Master][Link] slot=%u adv_id=%u state=boot mac=",
                      slave->slotId,
                      slave->advertisedSlaveId);
        printMac(macAddr);
        Serial.println();
      }
      return;
    }

    if (packet.type == PACKET_HEARTBEAT) {
      if (slave) {
        slave->lastHeartbeatMs = millis();
      }
      return;
    }

    if (packet.type != PACKET_NFC_READ && packet.type != PACKET_NFC_REMOVE) {
      return;
    }

    if (wasSeen(packet)) {
      EspNowPacket ack = packet;
      ack.type = PACKET_MASTER_ACK;
      ack.ackedType = packet.type;
      ack.masterRxMs = millis();
      esp_now_send(macAddr, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack));
      return;
    }

    rememberSeen(packet);

    const uint32_t recvMs = millis();
    const uint8_t effectiveSlaveId = slave ? slave->advertisedSlaveId : packet.slaveId;
    if (!shouldAcceptLogicalEvent(effectiveSlaveId, packet.nfcIndex, packet.type, packet.uid)) {
      EspNowPacket ack = packet;
      ack.type = PACKET_MASTER_ACK;
      ack.ackedType = packet.type;
      ack.masterRxMs = recvMs;
      esp_now_send(macAddr, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack));
      Serial.printf("[Master][Dedup] slave=%u nfc=%u event=%s uid=%s\n",
                    effectiveSlaveId,
                    packet.nfcIndex,
                    csvEventName(packet.type),
                    packet.uid);
      return;
    }

    CsvLogRow row = {};
    row.testId = g_nextTestId++;
    strncpy(row.phase, ExperimentConfig::kPhase, sizeof(row.phase) - 1);
    strncpy(row.environment, ExperimentConfig::kEnvironment, sizeof(row.environment) - 1);
    strncpy(row.pattern, ExperimentConfig::kPattern, sizeof(row.pattern) - 1);
    row.slaveId = effectiveSlaveId;
    row.nfcIndex = packet.nfcIndex;
    strncpy(row.uid, packet.uid, sizeof(row.uid) - 1);
    strncpy(row.eventName, csvEventName(packet.type), sizeof(row.eventName) - 1);
    row.bootId = packet.bootId;
    row.packetSeq = packet.seq;
    row.pollSeq = packet.pollSeq;
    row.retryCount = packet.attempt;
    row.masterPollIntervalMs = static_cast<uint16_t>(ExperimentConfig::kMasterPollIntervalMs);
    row.removeTimeMs = packet.removeTimeMs;
    row.nfcPollIntervalMs = packet.nfcPollIntervalMs;
    row.t0SlaveEventMs = packet.eventTimeMs;
    row.t1PollMs = packet.pollSentMs;
    row.t2RecvMs = recvMs;
    row.t3MqttMs = 0;
    xQueueSend(g_csvQueue, &row, 0);

    g_testCounters.totalEvents++;
    if (strncmp(row.eventName, "PICK-DOWN", sizeof(row.eventName)) == 0) {
      g_testCounters.pickDownEvents++;
    } else if (strncmp(row.eventName, "PICK-UP", sizeof(row.eventName)) == 0) {
      g_testCounters.pickUpEvents++;
    }
    if (isGoalEvent(row.eventName)) {
      if (isTimeBasedGoal() && g_testStartMs == 0) {
        g_testStartMs = recvMs;
        Serial.println("[Master][Count] timer started from first goal event");
      }
      g_testCounters.goalEventCount++;
      printCounterSummary("event");
      if (!isTimeBasedGoal() &&
          ExperimentConfig::kGoalTargetCount > 0 &&
          g_testCounters.goalEventCount >= ExperimentConfig::kGoalTargetCount) {
        Serial.println("[Master][Count] goal target reached");
      }
    }

    noteSlaveEvent(row.slaveId, row.eventName, recvMs);

    Serial.printf("[Master][Tag] %s %s test=%lu slave_slot=%u nfc=%u uid=%s\n",
                  eventEmojiByName(row.eventName),
                  row.eventName,
                  static_cast<unsigned long>(row.testId),
                  row.slaveId,
                  row.nfcIndex,
                  row.uid);

    EspNowPacket ack = packet;
    ack.type = PACKET_MASTER_ACK;
    ack.ackedType = packet.type;
    ack.masterRxMs = recvMs;
    esp_now_send(macAddr, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack));
  });

  esp_now_peer_info_t broadcastPeer = {};
  memcpy(broadcastPeer.peer_addr, BROADCAST_MAC, ESP_NOW_ETH_ALEN);
  // Channel 0 means "use the current home channel".
  // This is required when the master stays connected to a Wi-Fi AP and
  // slaves discover the master channel from heartbeats.
  broadcastPeer.channel = 0;
  broadcastPeer.ifidx = WIFI_IF_STA;
  broadcastPeer.encrypt = false;
  esp_now_add_peer(&broadcastPeer);
  Serial.printf("[Master][ESP-NOW] broadcast peer bound to current Wi-Fi channel=%d\n",
                WiFi.channel());
}

void sendHeartbeat() {
  if (millis() - g_lastHeartbeatMs < ExperimentConfig::kHeartbeatIntervalMs) {
    return;
  }
  g_lastHeartbeatMs = millis();

  MasterHeartbeat hb = {};
  hb.type = PACKET_HEARTBEAT;
  hb.channel = WiFi.channel();
  hb.seq = g_heartbeatSeq++;
  hb.uptimeSec = millis() / 1000;
  esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t*>(&hb), sizeof(hb));
}

void sendNextPoll() {
  if (millis() - g_lastPollMs < ExperimentConfig::kMasterPollIntervalMs) {
    return;
  }
  if (millis() < g_pollBackoffUntilMs) {
    return;
  }
  if (g_pollInFlight) {
    if (millis() - g_pollInFlightSinceMs < POLL_TX_TIMEOUT_MS) {
      return;
    }
    g_pollInFlight = false;
  }
  g_lastPollMs = millis();

  for (size_t checked = 0; checked < MAX_KNOWN_SLAVES; ++checked) {
    KnownSlave& slave = g_knownSlaves[g_nextPollIndex];
    g_nextPollIndex = (g_nextPollIndex + 1) % MAX_KNOWN_SLAVES;

    if (!slave.inUse || slave.isConsideredLost) {
      continue;
    }

    EspNowPacket poll = {};
    poll.version = PACKET_VERSION;
    poll.type = PACKET_MASTER_POLL;
    poll.slaveId = slave.advertisedSlaveId;
    poll.seq = g_pollSeq++;
    poll.pollSeq = poll.seq;
    poll.pollSentMs = millis();

    ensurePeerExists(slave.mac);
    const esp_err_t sendResult =
        esp_now_send(slave.mac, reinterpret_cast<const uint8_t*>(&poll), sizeof(poll));
    if (sendResult == ESP_OK) {
      g_pollInFlight = true;
      g_pollInFlightSinceMs = millis();
      memcpy(g_pollInFlightMac, slave.mac, ESP_NOW_ETH_ALEN);
    } else if (sendResult == ESP_ERR_ESPNOW_NO_MEM) {
      g_pollBackoffUntilMs = millis() + POLL_NO_MEM_BACKOFF_MS;
    }
    return;
  }
}

void checkSlaveStatus() {
  const uint32_t now = millis();
  for (KnownSlave& slave : g_knownSlaves) {
    if (!slave.inUse || slave.isConsideredLost) {
      continue;
    }
    if (now - slave.lastSeenMs > SLAVE_TIMEOUT_MS) {
      slave.isConsideredLost = true;
      slave.onlineAnnounced = false;
      Serial.printf("[Master][Link] slot=%u adv_id=%u state=lost\n", slave.slotId, slave.advertisedSlaveId);
    }
  }

  if (now - g_lastTimerSummaryMs >= STATE_LOG_INTERVAL_MS) {
      g_lastTimerSummaryMs = now;
      printMasterStateSummary();
      printPerSlaveEventSummary();
      if (isTimeBasedGoal()) {
        printCounterSummary("timer");
      }
      if (isTimeBasedGoal() && g_testStartMs != 0 && now - g_testStartMs >= ExperimentConfig::kGoalDurationMs) {
        Serial.println("[Master][Count] goal duration reached");
      }
  }
}

void flushCsvQueueToSerial() {
  CsvLogRow row = {};
  if (xQueueReceive(g_csvQueue, &row, 0) != pdTRUE) {
    return;
  }

  char topic[64];
  snprintf(topic, sizeof(topic), "no_display/%u/status", row.slaveId);

  char payload[160];
  snprintf(payload, sizeof(payload),
           "{\"event\":\"%s\",\"nfcIndex\":%u,\"uid\":\"%s\",\"testId\":%lu}",
           strcmp(row.eventName, "PICK-DOWN") == 0 ? "READ" : "REMOVE",
           row.nfcIndex,
           row.uid,
           static_cast<unsigned long>(row.testId));

  bool publishOk = false;
  if (g_mqttClient.connected()) {
    publishOk = g_mqttClient.publish(topic, payload);
  }

  if (publishOk) {
    g_testCounters.publishSuccess++;
  } else {
    g_testCounters.publishFail++;
  }

  row.t3MqttMs = millis();
  const uint32_t latencyQueueWait = row.t1PollMs >= row.t0SlaveEventMs ? row.t1PollMs - row.t0SlaveEventMs : 0;
  const uint32_t latencyRadio = row.t2RecvMs >= row.t1PollMs ? row.t2RecvMs - row.t1PollMs : 0;
  const uint32_t latencyWifi = row.t3MqttMs >= row.t2RecvMs ? row.t3MqttMs - row.t2RecvMs : 0;
  const uint32_t totalLatency = row.t3MqttMs >= row.t0SlaveEventMs ? row.t3MqttMs - row.t0SlaveEventMs : 0;

  Serial.printf("CSV,%lu,%s,%s,%s,%u,%u,%s,%s,%lu,%lu,%lu,%u,%u,%u,%u,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%s\n",
                static_cast<unsigned long>(row.testId),
                row.phase,
                row.environment,
                row.pattern,
                row.slaveId,
                row.nfcIndex,
                row.uid,
                row.eventName,
                static_cast<unsigned long>(row.bootId),
                static_cast<unsigned long>(row.packetSeq),
                static_cast<unsigned long>(row.pollSeq),
                row.retryCount,
                row.masterPollIntervalMs,
                row.removeTimeMs,
                row.nfcPollIntervalMs,
                static_cast<unsigned long>(row.t0SlaveEventMs),
                static_cast<unsigned long>(row.t1PollMs),
                static_cast<unsigned long>(row.t2RecvMs),
                static_cast<unsigned long>(row.t3MqttMs),
                static_cast<unsigned long>(latencyQueueWait),
                static_cast<unsigned long>(latencyRadio),
                static_cast<unsigned long>(latencyWifi),
                static_cast<unsigned long>(totalLatency),
                publishOk ? "SUCCESS" : "FAIL");
}

}  // namespace

void setup() {
  Serial.begin(BAUD);
  Serial.setDebugOutput(true);
  delay(1500);

  WiFi.mode(WIFI_STA);
  connectWiFi();

  g_mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  g_csvQueue = xQueueCreate(32, sizeof(CsvLogRow));

  Serial.println();
  Serial.println("=================================");
  Serial.println(" ESP-NOW MASTER TEST LOGGER");
  Serial.print(" Master MAC = ");
  Serial.println(WiFi.macAddress());
  Serial.printf(" Config: phase=%s env=%s pattern=%s poll=%lu remove=%d nfc_poll=%u goal=%s\n",
                ExperimentConfig::kPhase,
                ExperimentConfig::kEnvironment,
                ExperimentConfig::kPattern,
                static_cast<unsigned long>(ExperimentConfig::kMasterPollIntervalMs),
                ExperimentConfig::kRemoveTimeMs,
                ExperimentConfig::kNfcPollIntervalMs,
                ExperimentConfig::kGoalEvent);
  Serial.println(" CSV schema:");
  Serial.println(" CSV,Test_ID,Phase,Environment,Pattern,Slave_ID,NFC_Index,UID,Event,Boot_ID,Packet_Seq,Poll_Seq,Retry_Count,MasterPollIntervalMs,RemoveTimeMs,NfcPollIntervalMs,T0_SlaveEvent,T1_Poll,T2_Recv,T3_MQTT,Latency_QueueWait,Latency_Radio,Latency_Wifi,Total_Latency,Status");
  if (isTimeBasedGoal()) {
    char target[16];
    formatDuration(ExperimentConfig::kGoalDurationMs, target, sizeof(target));
    Serial.printf(" Goal event = %s, target duration = %s\n",
                  ExperimentConfig::kGoalEvent,
                  target);
  } else {
    Serial.printf(" Goal event = %s, target count = %lu\n",
                  ExperimentConfig::kGoalEvent,
                  static_cast<unsigned long>(ExperimentConfig::kGoalTargetCount));
  }
  Serial.println(" Notes:");
  Serial.println("  - Change espnow/common/ExperimentConfig.h before each run.");
  Serial.println("  - For hold tests, use SlaveSummary pickup count as false REMOVE count per slave.");
  Serial.println("=================================");
  printCounterSummary("boot");
  printPerSlaveEventSummary();

  setupEspNow();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!g_mqttClient.connected()) {
      connectMqtt();
    }
    g_mqttClient.loop();
  }

  sendHeartbeat();
  sendNextPoll();
  checkSlaveStatus();
  flushCsvQueueToSerial();
}
