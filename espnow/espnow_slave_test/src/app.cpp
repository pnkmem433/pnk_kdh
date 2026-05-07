#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <nfc.h>
#include "EspNowLink.h"
#include "../../common/ExperimentConfig.h"

extern NfcReader nfcReader;
extern NfcReader nfcReader2;
extern TaskRunner muxTask;

namespace {

constexpr uint32_t BAUD = 115200;
constexpr uint32_t POWER_STABILIZE_MS = 2000;
constexpr uint32_t NFC_STABILIZE_MS = 700;
constexpr uint32_t BOOT_JITTER_MAX_MS = 400;

uint8_t g_slaveId = 0;
String g_lastUid1;
String g_lastUid2;

}  // namespace

void setup() {
  Serial.begin(BAUD);
  Serial.setTxTimeoutMs(0); // USB 미연결 시 Serial.print 무한 대기(멈춤) 방지
  Serial.setDebugOutput(true);
  delay(POWER_STABILIZE_MS);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE); // [요구사항 1] Wi-Fi 절전 모드 강제 해제 (채널 스캔 중 수면 방지)
  uint8_t macBytes[6] = {};
  WiFi.macAddress(macBytes);
  g_slaveId = macBytes[5];
  uint32_t bootId = esp_random();
  const uint32_t bootJitterMs = g_slaveId % BOOT_JITTER_MAX_MS;

  Serial.println();
  Serial.println("=== ESP-NOW SLAVE NFC TEST START ===");
  Serial.printf("[Slave-%u] self_mac=%s\n", g_slaveId, WiFi.macAddress().c_str());
  Serial.printf("[Slave-%u] config remove=%dms nfc_poll=%ums mux_switch=%lums\n",
                g_slaveId,
                ExperimentConfig::kRemoveTimeMs,
                ExperimentConfig::kNfcPollIntervalMs,
                static_cast<unsigned long>(ExperimentConfig::kMuxSwitchIntervalMs));

  delay(bootJitterMs);
  EspNowLink::getInstance().begin(g_slaveId, bootId, 1);
  EspNowLink::getInstance().setTimingSnapshot(nfcReader.pollIntervalMs(), nfcReader.removeTimeMs());

  delay(NFC_STABILIZE_MS);

  nfcReader.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid1 = uidStr;
            Serial.println("\n==================================================");
            Serial.printf(" 🟢 [NFC-1] 태그 인식됨 (READ) - UID: %s\n", uidStr.c_str());
            Serial.println("==================================================");
            EspNowLink::getInstance().queueNfcPacket(1, PACKET_NFC_READ, uidStr);
          },
      .onRemove =
          []() {
            if (g_lastUid1.isEmpty()) {
              return;
            }
            Serial.println("\n--------------------------------------------------");
            Serial.printf(" 🔴 [NFC-1] 태그 제거됨 (REMOVE)\n");
            Serial.println("--------------------------------------------------");
            EspNowLink::getInstance().queueNfcPacket(1, PACKET_NFC_REMOVE, g_lastUid1);
            g_lastUid1 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("⚠️ [ERROR] NFC-1 통신 오류 (배선 점검 필요) - %s\n", err.c_str());
          },
  });

  delay(50); // [수정] 두 리더기가 동시에 초기화되면서 SPI 버스가 엉키는 것을 방지

  // NFC-2 초기화 복구 (진단용)
  nfcReader2.begin({
      .onRead =
          [](const String& uidStr) {
            g_lastUid2 = uidStr;
            Serial.println("\n==================================================");
            Serial.printf(" 🟢 [NFC-2] 태그 인식됨 (READ) - UID: %s\n", uidStr.c_str());
            Serial.println("==================================================");
            EspNowLink::getInstance().queueNfcPacket(2, PACKET_NFC_READ, uidStr);
          },
      .onRemove =
          []() {
            if (g_lastUid2.isEmpty()) {
              return;
            }
            Serial.println("\n--------------------------------------------------");
            Serial.printf(" 🔴 [NFC-2] 태그 제거됨 (REMOVE)\n");
            Serial.println("--------------------------------------------------");
            EspNowLink::getInstance().queueNfcPacket(2, PACKET_NFC_REMOVE, g_lastUid2);
            g_lastUid2 = "";
          },
      .onError =
          [](const String& err) {
            Serial.printf("⚠️ [ERROR] NFC-2 통신 오류 (배선 점검 필요) - %s\n", err.c_str());
          },
  });

  EspNowLink::getInstance().setTimingSnapshot(nfcReader.pollIntervalMs(), nfcReader.removeTimeMs());

  nfcReader.setEnabled(true);  // NFC-1 다시 켜기
  nfcReader2.setEnabled(false); // 초기 상태는 1번 켜짐, 2번 꺼짐으로 세팅

  // 스위칭 로직 복구 (진단용)
  muxTask.begin({
      .loop =
          []() {
            static uint32_t lastSwitchMs = 0;
            static bool firstActive = true;
            const uint32_t now = millis();

            // [수정] 스위칭 주기를 250ms로 늘려 카드가 충분한 전력을 공급받고 인식될 시간을 줌
            if (now - lastSwitchMs < ExperimentConfig::kMuxSwitchIntervalMs) {
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
  EspNowLink::getInstance().loop();
}
