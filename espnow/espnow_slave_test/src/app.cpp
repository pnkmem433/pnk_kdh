#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <nfc.h>

extern NfcReader nfcReader;
extern NfcReader nfcReader2;
extern TaskRunner muxTask;

namespace {

constexpr uint32_t BAUD = 115200;
constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint8_t PACKET_VERSION = 1;

// Master MAC Address: 68:67:25:EC:A5:84
uint8_t MASTER_MAC[6] = {0x68, 0x67, 0x25, 0xEC, 0xA5, 0x84};

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
  char uid[32];
};

uint32_t g_nextSeq = 1;
uint8_t g_slaveId = 0;
String g_lastUid1;
String g_lastUid2;

void printMac(const uint8_t* mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool isMasterMacConfigured() {
  for (uint8_t value : MASTER_MAC) {
    if (value != 0x00) {
      return true;
    }
  }
  return false;
}

void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  Serial.print("[SLAVE][SEND_CB] to ");
  printMac(macAddr);
  Serial.printf(" -> %s\n", status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

void onDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
  if (len != static_cast<int>(sizeof(EspNowPacket))) {
    Serial.printf("[SLAVE-%u] unexpected packet size=%d from ", g_slaveId, len);
    printMac(macAddr);
    Serial.println();
    return;
  }

  EspNowPacket packet = {};
  memcpy(&packet, incomingData, sizeof(packet));

  if (packet.version != PACKET_VERSION || packet.type != PACKET_MASTER_ACK) {
    Serial.printf("[SLAVE-%u] ignored packet type=%u version=%u\n",
                  g_slaveId, packet.type, packet.version);
    return;
  }

  const uint32_t rttUs = micros() - packet.sendTimeUs;
  Serial.printf("[SLAVE-%u][ACK] NFC-%u uid=%s seq=%lu rtt=%lu us\n",
                packet.slaveId,
                packet.nfcIndex,
                packet.uid,
                static_cast<unsigned long>(packet.seq),
                static_cast<unsigned long>(rttUs));
}

void sendNfcPacket(uint8_t nfcIndex, PacketType type, const String& uid) {
  if (!isMasterMacConfigured()) {
    Serial.println("[SLAVE] MASTER_MAC is not configured.");
    return;
  }

  EspNowPacket packet = {};
  packet.version = PACKET_VERSION;
  packet.type = type;
  packet.slaveId = g_slaveId;
  packet.nfcIndex = nfcIndex;
  packet.seq = g_nextSeq++;
  packet.sendTimeUs = micros();
  uid.toCharArray(packet.uid, sizeof(packet.uid));

  const esp_err_t err = esp_now_send(MASTER_MAC,
                                     reinterpret_cast<const uint8_t*>(&packet),
                                     sizeof(packet));
  Serial.printf("[SLAVE-%u][NFC %s] send NFC-%u uid=%s seq=%lu result=%s\n",
                g_slaveId,
                type == PACKET_NFC_READ ? "READ" : "REMOVE",
                nfcIndex,
                packet.uid,
                static_cast<unsigned long>(packet.seq),
                esp_err_to_name(err));
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(100);

  const esp_err_t startResult = esp_wifi_start();
  Serial.printf("[SLAVE-%u][WIFI] start result=%s\n", g_slaveId, esp_err_to_name(startResult));

  const esp_err_t channelResult = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.printf("[SLAVE-%u][WIFI] set channel=%u result=%s\n",
                g_slaveId, ESPNOW_CHANNEL, esp_err_to_name(channelResult));

  const esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    Serial.printf("[SLAVE-%u] esp_now_init failed: %s\n", g_slaveId, esp_err_to_name(initResult));
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, MASTER_MAC, sizeof(MASTER_MAC));
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  const esp_err_t peerResult = esp_now_add_peer(&peerInfo);
  Serial.print("[SLAVE] target MASTER MAC = ");
  printMac(MASTER_MAC);
  Serial.println();
  Serial.printf("[SLAVE-%u][PEER] add master result=%s\n", g_slaveId, esp_err_to_name(peerResult));
}

}  // namespace

void setup() {
  Serial.begin(BAUD);
  Serial.setDebugOutput(true);
  delay(1500);

  WiFi.mode(WIFI_STA);
  uint8_t macBytes[6] = {};
  WiFi.macAddress(macBytes);
  g_slaveId = macBytes[5];

  Serial.println();
  Serial.println("=== ESP-NOW SLAVE NFC TEST START ===");
  Serial.printf("[SLAVE-%u] self MAC = %s\n", g_slaveId, WiFi.macAddress().c_str());
  Serial.printf("[SLAVE-%u] channel = %u\n", g_slaveId, ESPNOW_CHANNEL);
  Serial.println("[SLAVE] pin map: NFC1 SS=D3, NFC2 SS=D4, RST=D2, SPI=D8/D9/D10");

  setupEspNow();

  nfcReader.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid1 = uidStr;
            Serial.printf("[SLAVE-%u][NFC READ] NFC-1 uid=%s\n", g_slaveId, uidStr.c_str());
            sendNfcPacket(1, PACKET_NFC_READ, uidStr);
          },
      .onRemove =
          []() {
            if (g_lastUid1.isEmpty()) {
              return;
            }
            Serial.printf("[SLAVE-%u][NFC REMOVE] NFC-1 uid=%s\n", g_slaveId, g_lastUid1.c_str());
            sendNfcPacket(1, PACKET_NFC_REMOVE, g_lastUid1);
            g_lastUid1 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("[SLAVE-%u] NFC-1 ERROR %s\n", g_slaveId, err.c_str());
          },
  });

  nfcReader2.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid2 = uidStr;
            Serial.printf("[SLAVE-%u][NFC READ] NFC-2 uid=%s\n", g_slaveId, uidStr.c_str());
            sendNfcPacket(2, PACKET_NFC_READ, uidStr);
          },
      .onRemove =
          []() {
            if (g_lastUid2.isEmpty()) {
              return;
            }
            Serial.printf("[SLAVE-%u][NFC REMOVE] NFC-2 uid=%s\n", g_slaveId, g_lastUid2.c_str());
            sendNfcPacket(2, PACKET_NFC_REMOVE, g_lastUid2);
            g_lastUid2 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("[SLAVE-%u] NFC-2 ERROR %s\n", g_slaveId, err.c_str());
          },
  });

  nfcReader.setEnabled(true);
  nfcReader2.setEnabled(false);

  muxTask.begin({
      .loop =
          []() {
            static uint32_t lastSwitchMs = 0;
            static bool firstActive = true;
            const uint32_t now = millis();

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

void loop() {
  static uint32_t lastAliveMs = 0;
  const uint32_t now = millis();

  if (now - lastAliveMs >= 5000) {
    lastAliveMs = now;
    Serial.printf("[SLAVE-%u] ALIVE ms=%lu\n", g_slaveId, static_cast<unsigned long>(now));
  }
}
