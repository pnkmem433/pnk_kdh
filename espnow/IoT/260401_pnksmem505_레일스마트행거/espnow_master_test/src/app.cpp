#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

namespace {

constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint32_t BAUD = 115200;

enum PacketType : uint8_t {
  PACKET_NFC_READ = 1,
  PACKET_NFC_REMOVE = 2,
  PACKET_MASTER_ACK = 3,
};

struct EspNowPacket {
  uint8_t version;
  uint8_t type;
  uint8_t slaveId;
  uint8_t nfcIndex;
  uint32_t seq;
  char uid[32];
};

void printMac(const uint8_t* mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void printSelfMac() {
  uint8_t mac[6] = {0};
  WiFi.macAddress(mac);
  Serial.print("[MASTER] MAC = ");
  printMac(mac);
  Serial.println();
}

void ensurePeer(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) {
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  esp_err_t err = esp_now_add_peer(&peerInfo);
  if (err != ESP_OK) {
    Serial.printf("[MASTER] add peer failed: %d\n", static_cast<int>(err));
  }
}

void sendAck(const uint8_t* targetMac, const EspNowPacket& packet) {
  ensurePeer(targetMac);

  EspNowPacket ack = {};
  ack.version = 1;
  ack.type = PACKET_MASTER_ACK;
  ack.slaveId = packet.slaveId;
  ack.nfcIndex = packet.nfcIndex;
  ack.seq = packet.seq;
  strncpy(ack.uid, packet.uid, sizeof(ack.uid) - 1);

  esp_err_t err = esp_now_send(targetMac, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack));
  Serial.printf("[MASTER] ACK send result = %d\n", static_cast<int>(err));
}

void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  Serial.print("[MASTER] send cb to ");
  printMac(macAddr);
  Serial.printf(" -> %s\n", status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

void onDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  if (len != static_cast<int>(sizeof(EspNowPacket))) {
    Serial.printf("[MASTER] unexpected packet size: %d\n", len);
    return;
  }

  EspNowPacket packet = {};
  memcpy(&packet, incomingData, sizeof(packet));

  Serial.print("[MASTER] from MAC ");
  printMac(info->src_addr);
  Serial.println();

  if (packet.type == PACKET_NFC_READ) {
    Serial.printf("[MASTER] SLAVE=%u NFC=%u UID=%s SEQ=%lu\n",
                  packet.slaveId,
                  packet.nfcIndex,
                  packet.uid,
                  static_cast<unsigned long>(packet.seq));
  } else if (packet.type == PACKET_NFC_REMOVE) {
    Serial.printf("[MASTER] SLAVE=%u NFC=%u REMOVED SEQ=%lu\n",
                  packet.slaveId,
                  packet.nfcIndex,
                  static_cast<unsigned long>(packet.seq));
  } else if (packet.type == PACKET_MASTER_ACK) {
    Serial.printf("[MASTER] unexpected ACK packet from slave=%u\n", packet.slaveId);
  } else {
    Serial.printf("[MASTER] unknown type=%u\n", packet.type);
  }

  sendAck(info->src_addr, packet);
}

}  // namespace

void setup() {
  Serial.begin(BAUD);
  delay(300);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.setChannel(ESPNOW_CHANNEL);

  Serial.println();
  Serial.println("=== ESP-NOW MASTER TEST START ===");
  Serial.printf("[MASTER] channel = %u\n", ESPNOW_CHANNEL);
  printSelfMac();

  esp_err_t err = esp_now_init();
  if (err != ESP_OK) {
    Serial.printf("[MASTER] esp_now_init failed: %d\n", static_cast<int>(err));
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("[MASTER] ready");
}

void loop() {
  delay(100);
}
