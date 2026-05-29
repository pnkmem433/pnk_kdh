#if 0
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <nfc.h>

extern NfcReader nfcReader;
extern NfcReader nfcReader2;
extern TaskRunner muxTask;

namespace {

constexpr uint32_t BAUD = 115200;
constexpr uint8_t ESPNOW_CHANNEL = 1;

#ifndef SLV_ID
#define SLV_ID 1  // 빌드 플래그가 없으면 기본값 1을 사용
#endif
constexpr uint8_t SLAVE_ID = SLV_ID;

// TODO: 여기에 Master의 실제 MAC 주소를 입력하세요. 예: {0x34, 0x85, 0x18, 0x00, 0x11, 0x22}
static uint8_t MASTER_MAC[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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

uint32_t g_nextSeq = 1;
String g_lastUid1;
String g_lastUid2;

void printMac(const uint8_t* mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void printSelfMac() {
  uint8_t mac[6] = {0};
  WiFi.macAddress(mac);
  Serial.print("[SLAVE] self MAC = ");
  printMac(mac);
  Serial.println();
}

bool isMasterMacConfigured() {
  for (uint8_t value : MASTER_MAC) {
    if (value != 0x00) {
      return true;
    }
  }
  return false;
}

void printConfiguredMasterMac() {
  Serial.print("[SLAVE] target MASTER MAC = ");
  printMac(MASTER_MAC);
  Serial.println();
}

void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  Serial.print("[SLAVE] send cb to ");
  printMac(macAddr);
  Serial.printf(" -> %s\n", status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

void onDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  if (len != static_cast<int>(sizeof(EspNowPacket))) {
    Serial.printf("[SLAVE] unexpected packet size: %d\n", len);
    return;
  }

  EspNowPacket packet = {};
  memcpy(&packet, incomingData, sizeof(packet));

  if (packet.type != PACKET_MASTER_ACK) {
    Serial.printf("[SLAVE] recv non-ack type=%u\n", packet.type);
    return;
  }

  Serial.print("[SLAVE] ACK from ");
  printMac(info->src_addr);
  Serial.printf(" | slave=%u nfc=%u seq=%lu uid=%s\n",
                packet.slaveId,
                packet.nfcIndex,
                static_cast<unsigned long>(packet.seq),
                packet.uid);
}

void sendNfcPacket(uint8_t nfcIndex, PacketType type, const String& uid) {
  if (!isMasterMacConfigured()) {
    Serial.println("[SLAVE] MASTER_MAC not set. Update app.cpp first.");
    return;
  }

  EspNowPacket packet = {};
  packet.version = 1;
  packet.type = type;
  packet.slaveId = SLAVE_ID;
  packet.nfcIndex = nfcIndex;
  packet.seq = g_nextSeq++;
  uid.toCharArray(packet.uid, sizeof(packet.uid));

  esp_err_t err = esp_now_send(MASTER_MAC, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

  Serial.printf("[SLAVE] send type=%u slave=%u nfc=%u seq=%lu uid=%s result=%d\n",
                packet.type,
                packet.slaveId,
                packet.nfcIndex,
                static_cast<unsigned long>(packet.seq),
                packet.uid,
                static_cast<int>(err));
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.setChannel(ESPNOW_CHANNEL);

  printSelfMac();
  printConfiguredMasterMac();

  esp_err_t err = esp_now_init();
  if (err != ESP_OK) {
    Serial.printf("[SLAVE] esp_now_init failed: %d\n", static_cast<int>(err));
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  if (isMasterMacConfigured()) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, MASTER_MAC, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;

    err = esp_now_add_peer(&peerInfo);
    Serial.printf("[SLAVE] add master peer result = %d\n", static_cast<int>(err));
  }
}

}  // namespace

void setup() {
  Serial.begin(BAUD);
  delay(300);

  Serial.println();
  Serial.println("=== ESP-NOW SLAVE NFC TEST START ===");
  Serial.printf("[SLAVE] slave id = %u\n", SLAVE_ID);
  Serial.printf("[SLAVE] channel = %u\n", ESPNOW_CHANNEL);

  setupEspNow();

  nfcReader.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid1 = uidStr;
            Serial.printf("[SLAVE] NFC1 READ uid=%s\n", uidStr.c_str());
            sendNfcPacket(1, PACKET_NFC_READ, uidStr);
          },
      .onRemove =
          []() {
            if (g_lastUid1.isEmpty()) {
              return;
            }
            Serial.println("[SLAVE] NFC1 REMOVED");
            sendNfcPacket(1, PACKET_NFC_REMOVE, g_lastUid1);
            g_lastUid1 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("[SLAVE] NFC1 ERROR %s\n", err.c_str());
          },
  });

  nfcReader2.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid2 = uidStr;
            Serial.printf("[SLAVE] NFC2 READ uid=%s\n", uidStr.c_str());
            sendNfcPacket(2, PACKET_NFC_READ, uidStr);
          },
      .onRemove =
          []() {
            if (g_lastUid2.isEmpty()) {
              return;
            }
            Serial.println("[SLAVE] NFC2 REMOVED");
            sendNfcPacket(2, PACKET_NFC_REMOVE, g_lastUid2);
            g_lastUid2 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("[SLAVE] NFC2 ERROR %s\n", err.c_str());
          },
  });

  nfcReader.setEnabled(true);
  nfcReader2.setEnabled(false);

  muxTask.begin({
      .loop =
          []() {
            static uint32_t lastSwitchMs = 0;
            static bool firstActive = true;
            uint32_t now = millis();

            if (now - lastSwitchMs < 200) {
              return;
            }

            lastSwitchMs = now;
            firstActive = !firstActive;
            nfcReader.setEnabled(firstActive);
            nfcReader2.setEnabled(!firstActive);
          },
  });
}

void loop() {}
#endif