#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_ota_ops.h>

#include "AppConfig.h"
#include "BoardConfig.h"

namespace {

constexpr unsigned long kWifiRetryDelayMs = 100;
constexpr size_t kDownloadBufferSize = 1024;

void writeStatusLed(bool on) {
  digitalWrite(STATUS_LED_PIN, on ? LOW : HIGH);
}

void tickStatusLed(unsigned long intervalMs) {
  static unsigned long lastToggleMs = 0;
  static bool ledOn = false;
  const unsigned long now = millis();
  if (now - lastToggleMs >= intervalMs) {
    lastToggleMs = now;
    ledOn = !ledOn;
    writeStatusLed(ledOn);
  }
}

String buildMigrationMainUrl() {
  return String(OTA_URL) + "/firmwareDownload/migration-main?projectId=" +
         String(OTA_PROJECT_ID) + "&chipType=" + String(CHIP_TYPE);
}

void waitForWifi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("[migration] WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(kWifiRetryDelayMs);
    tickStatusLed(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("[migration] WiFi connected");
  Serial.print("[migration] Local IP: ");
  Serial.println(WiFi.localIP());
}

bool downloadMainTasmota() {
  WiFiClient client;
  HTTPClient http;
  const String mainUrl = buildMigrationMainUrl();
  const esp_partition_t *targetPartition =
      esp_ota_get_next_update_partition(nullptr);

  Serial.print("[migration] Main URL: ");
  Serial.println(mainUrl);
  if (targetPartition == nullptr) {
    Serial.println("[migration] No app partition for main firmware");
    return false;
  }
  Serial.printf("[migration] Target partition: %s / %u bytes\n",
                targetPartition->label, targetPartition->size);

  if (!http.begin(client, mainUrl)) {
    Serial.println("[migration] Failed to begin main download");
    return false;
  }
  http.setReuse(false);
  http.setTimeout(15000);

  const int responseCode = http.GET();
  Serial.printf("[migration] Main response code: %d\n", responseCode);
  if (responseCode != 200) {
    http.end();
    return false;
  }

  const int totalSize = http.getSize();
  Serial.printf("[migration] Main size: %d\n", totalSize);
  if (totalSize <= 0) {
    http.end();
    return false;
  }
  if (static_cast<size_t>(totalSize) > targetPartition->size) {
    Serial.println("[migration] Main firmware is too large for app0");
    http.end();
    return false;
  }

  if (!Update.begin(static_cast<size_t>(totalSize), U_FLASH)) {
    Serial.println("[migration] Update.begin failed");
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[kDownloadBufferSize];
  size_t written = 0;
  float lastProgress = -1.0f;

  while (http.connected() && written < static_cast<size_t>(totalSize)) {
    const size_t available = stream->available();
    if (!available) {
      delay(1);
      tickStatusLed(80);
      continue;
    }

    const size_t toRead = min(kDownloadBufferSize, available);
    const size_t readCount = stream->readBytes(buffer, toRead);
    if (readCount == 0) {
      continue;
    }

    const size_t chunkWritten = Update.write(buffer, readCount);
    if (chunkWritten != readCount) {
      Serial.println("[migration] Update.write failed");
      Update.abort();
      http.end();
      return false;
    }

    written += chunkWritten;
    const float progress =
        static_cast<float>(written) / static_cast<float>(totalSize);
    if (progress - lastProgress >= 0.05f || progress >= 1.0f) {
      lastProgress = progress;
      Serial.printf("[migration] Progress: %.0f%%\n", progress * 100.0f);
    }
    tickStatusLed(80);
  }

  http.end();

  if (written != static_cast<size_t>(totalSize)) {
    Serial.println("[migration] Incomplete download");
    Update.abort();
    return false;
  }

  if (!Update.end(true)) {
    Serial.printf("[migration] Update.end failed: %s\n", Update.errorString());
    return false;
  }
  if (esp_ota_set_boot_partition(targetPartition) != ESP_OK) {
    Serial.println("[migration] Failed to activate main partition");
    return false;
  }

  Serial.println("[migration] Main Tasmota written successfully");
  return true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- Migration Safeboot Start ---");

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);

  waitForWifi();

  if (downloadMainTasmota()) {
    Serial.println("[migration] Restarting into main Tasmota");
    delay(500);
    ESP.restart();
  }

  Serial.println("[migration] Migration failed, staying in safeboot loop");
}

void loop() {
  static unsigned long lastToggleMs = 0;
  static bool ledOn = false;
  const unsigned long now = millis();
  if (now - lastToggleMs >= 150) {
    lastToggleMs = now;
    ledOn = !ledOn;
    digitalWrite(STATUS_LED_PIN, ledOn ? LOW : HIGH);
  }
}
