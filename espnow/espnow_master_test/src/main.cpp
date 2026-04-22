#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace {

constexpr uint32_t BAUD = 115200;
constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint8_t PACKET_VERSION = 1;
constexpr size_t UID_MAX_LEN = 32;

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
  uint32_t sendTimeUs;
  char uid[UID_MAX_LEN];
};

void printMac(const uint8_t* mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void ensurePeerExists(const uint8_t* macAddr) {
  if (esp_now_is_peer_exist(macAddr)) {
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macAddr, ESP_NOW_ETH_ALEN);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  const esp_err_t result = esp_now_add_peer(&peerInfo);
  Serial.print("[MASTER] add peer ");
  printMac(macAddr);
  Serial.printf(" -> %s\n", esp_err_to_name(result));
}

void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  Serial.print("[MASTER] ACK send callback to ");
  printMac(macAddr);
  Serial.printf(" -> %s\n", status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

void onDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
  if (len != static_cast<int>(sizeof(EspNowPacket))) {
    Serial.print("[MASTER] unexpected packet size from ");
    printMac(macAddr);
    Serial.printf(": len=%d expected=%u\n", len, static_cast<unsigned>(sizeof(EspNowPacket)));
    return;
  }

  EspNowPacket packet = {};
  memcpy(&packet, incomingData, sizeof(packet));

  if (packet.version != PACKET_VERSION) {
    Serial.printf("[MASTER] ignored packet version=%u from ", packet.version);
    printMac(macAddr);
    Serial.println();
    return;
  }

  if (packet.type != PACKET_NFC_READ && packet.type != PACKET_NFC_REMOVE) {
    Serial.printf("[MASTER] ignored packet type=%u slave=%u seq=%lu\n",
                  packet.type,
                  packet.slaveId,
                  static_cast<unsigned long>(packet.seq));
    return;
  }

  const uint32_t rxTimeUs = micros();
  const char* action = packet.type == PACKET_NFC_READ ? "READ" : "REMOVE";

  Serial.println();
  Serial.println("---------------------------------------------------------");
  Serial.printf("[MASTER][NFC %s] RX from MAC=", action);
  printMac(macAddr);
  Serial.println();
  Serial.printf("[MASTER][NFC %s] SLAVE-%u NFC-%u uid=%s seq=%lu rxTime=%lu us\n",
                action,
                packet.slaveId,
                packet.nfcIndex,
                packet.uid,
                static_cast<unsigned long>(packet.seq),
                static_cast<unsigned long>(rxTimeUs));

  ensurePeerExists(macAddr);

  packet.type = PACKET_MASTER_ACK;
  const esp_err_t ackResult = esp_now_send(macAddr,
                                           reinterpret_cast<const uint8_t*>(&packet),
                                           sizeof(packet));
  Serial.printf("[MASTER][ACK] send request -> %s\n", esp_err_to_name(ackResult));
  Serial.println("---------------------------------------------------------");
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(100);

  const esp_err_t startResult = esp_wifi_start();
  Serial.printf("[MASTER][WIFI] start result=%s\n", esp_err_to_name(startResult));

  const esp_err_t channelResult = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.printf("[MASTER][WIFI] set channel=%u result=%s\n",
                ESPNOW_CHANNEL, esp_err_to_name(channelResult));

  const esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    Serial.printf("[MASTER] esp_now_init failed: %s\n", esp_err_to_name(initResult));
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
}

}  // namespace

void setup() {
  Serial.begin(BAUD);
  Serial.setDebugOutput(true);
  delay(1500);

  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("=================================");
  Serial.println("   ESP-NOW MASTER START");
  Serial.print("   self MAC = ");
  Serial.println(WiFi.macAddress());
  Serial.println("=================================");

  setupEspNow();

  Serial.println("[MASTER] waiting for slave NFC packets...");
}

void loop() {
  static uint32_t lastAliveMs = 0;
  const uint32_t now = millis();

  if (now - lastAliveMs >= 5000) {
    lastAliveMs = now;
    Serial.printf("[MASTER] ALIVE ms=%lu\n", static_cast<unsigned long>(now));
  }
}
