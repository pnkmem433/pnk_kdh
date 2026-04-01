#include <Arduino.h>
#include <MFRC522.h>
#include <QRcode_eSPI.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Ui.h>
#include <Wire.h>
#include <WifiManager.h>
#include <esp_rom_sys.h>
#include <uuid.h>

#define SERIAL_BAUD_RATE 115200
#define TFT_CS_PIN D1
#define TFT_DC_PIN D3
#define TFT_BL_PIN D6
#define CHARGE_PIN D7
#define NFC_SS_PIN D0
#define NFC_RST_PIN D2
#define I2C_SDA_PIN D4
#define I2C_SCL_PIN D5
#define SPI_SCK_PIN D8
#define SPI_MISO_PIN D9
#define SPI_MOSI_PIN D10
#define POLL_INTERVAL_MS 60
#define REMOVE_CONFIRM_COUNT 3
#define WIFI_BANNER_MS 1500UL
#define INA_SAMPLE_MS 1000UL
// INA226-compatible monitor registers used for battery voltage/current checks.
#define INA_REG_CONFIG 0x00
#define INA_REG_VSHUNT 0x01
#define INA_REG_VBUS 0x02
#define INA_REG_MANUFACTURER_ID 0xFE
#define INA_REG_DEVICE_ID 0xFF

TFT_eSPI tft;
TFT_eSprite spr(&tft);
TFT_eSprite qrSpr(&tft);
QRcode_eSPI qrcode(&qrSpr);
Ui ui(&tft, &spr, &qrSpr, &qrcode);
MFRC522 mfrc522(NFC_SS_PIN, NFC_RST_PIN);
Uuid uuid = Uuid();
WifiManager wifi = WifiManager({
    {.ssid = "fxco-pnksolution-stylingbooth", .password = "71805802"},
    {.ssid = "CC-Retail", .password = "pnks1111"},
});

namespace {
enum class ScreenMode {
  Idle,
  Detected,
  Removed,
  WifiSuccess,
};

bool uiReady = false;
bool cardPresent = false;
bool lastReadFailed = false;
bool wifiConnected = false;
bool wifiStateKnown = false;
bool wifiBannerPending = false;
bool chargerInputPresent = false;
bool chargeConnected = false;
bool chargeStateKnown = false;
bool inaI2cDetected = false;
bool inaDataReady = false;
bool inaIdentified = false;
uint8_t missingReadCount = 0;
uint8_t inaAddress = 0;
uint16_t inaManufacturerId = 0;
uint16_t inaDeviceId = 0;
uint16_t inaConfigRaw = 0;
String currentUid = "";
String inaStatusText = "INA: SCAN";
unsigned long lastPollAt = 0;
unsigned long wifiBannerStartedAt = 0;
unsigned long lastInaSampleAt = 0;
unsigned long lastBatteryPercentStepAt = 0;
unsigned long chargeSessionStartedAt = 0;
float inaBusVoltage = 0.0f;
float inaShuntVoltageMv = 0.0f;
float inaCurrentMa = 0.0f;
float inaSignedCurrentMa = 0.0f;
uint16_t inaRawBus = 0;
int16_t inaRawShunt = 0;
int inaBatteryPercent = -1;
int inaBatteryPercentRaw = -1;
int pendingBatteryPercent = -1;
uint8_t pendingBatteryCount = 0;
float chargeStartVoltage = 0.0f;
float chargePeakVoltage = 0.0f;
ScreenMode currentScreen = ScreenMode::Idle;

void refreshStatusIndicators();
void renderCurrentScreen();

bool isChargerInputPresent(bool rawState) {
  // Use an internal pull-up and treat the sensed charger line as active-low.
  return !rawState;
}

bool isBatteryActuallyCharging() {
  if (!chargerInputPresent || !inaDataReady) {
    return false;
  }

  // With the current INA wiring, discharging shows a positive signed current.
  // Charging should flip the direction and become negative.
  return inaSignedCurrentMa <= -5.0f;
}

void updateDisplayedBatteryPercent(int rawPercent) {
  if (rawPercent < 0) {
    return;
  }

  const unsigned long now = millis();

  if (inaBatteryPercent < 0) {
    inaBatteryPercent = rawPercent;
    pendingBatteryPercent = rawPercent;
    pendingBatteryCount = 0;
    lastBatteryPercentStepAt = now;
    return;
  }

  if (chargeConnected) {
    if (rawPercent > inaBatteryPercent &&
        now - lastBatteryPercentStepAt >= 2500UL) {
      inaBatteryPercent++;
      lastBatteryPercentStepAt = now;
    }
    pendingBatteryPercent = rawPercent;
    pendingBatteryCount = 0;
    return;
  }

  if (rawPercent >= inaBatteryPercent) {
    pendingBatteryPercent = rawPercent;
    pendingBatteryCount = 0;
    return;
  }

  if (rawPercent != pendingBatteryPercent) {
    pendingBatteryPercent = rawPercent;
    pendingBatteryCount = 1;
    return;
  }

  if (pendingBatteryCount < 3) {
    pendingBatteryCount++;
  }

  if (pendingBatteryCount >= 3) {
    inaBatteryPercent--;
    if (inaBatteryPercent < rawPercent) {
      inaBatteryPercent = rawPercent;
    }
    pendingBatteryCount = 0;
    lastBatteryPercentStepAt = now;
  }
}

void applyChargingState(bool newState, const char *reason) {
  if (chargeStateKnown && newState == chargeConnected) {
    return;
  }

  chargeConnected = newState;
  chargeStateKnown = true;
  ui.setCharge(chargeConnected);
  if (chargeConnected) {
    chargeSessionStartedAt = millis();
    chargeStartVoltage = inaBusVoltage;
    chargePeakVoltage = inaBusVoltage;
  }
  Serial.printf("[CHARGE] %s -> %s\r\n", reason,
                chargeConnected ? "CHARGING" : "NOT CHARGING");
  refreshStatusIndicators();
}

void beginSharedSpi() {
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
}

void selectDisplay() {
  digitalWrite(NFC_SS_PIN, HIGH);
}

void selectNfc() {
  digitalWrite(TFT_CS_PIN, HIGH);
}

void drawWifiIndicator() {
  if (!uiReady) {
    return;
  }

  const int cx = 82;
  const int cy = 170;
  const uint16_t color = wifiConnected ? TFT_BLUE : TFT_DARKGREY;
  const uint16_t bg = TFT_BLACK;

  tft.fillRect(44, 148, 76, 54, bg);
  tft.fillCircle(cx, cy + 8, 2, color);
  tft.drawLine(cx - 5, cy + 4, cx, cy, color);
  tft.drawLine(cx, cy, cx + 5, cy + 4, color);
  tft.drawLine(cx - 10, cy, cx, cy - 7, color);
  tft.drawLine(cx, cy - 7, cx + 10, cy, color);
  tft.drawLine(cx - 14, cy - 4, cx, cy - 13, color);
  tft.drawLine(cx, cy - 13, cx + 14, cy - 4, color);

  if (!wifiConnected) {
    tft.drawLine(cx - 15, cy - 12, cx + 15, cy + 11, color);
  }

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(color, bg);
  tft.setTextSize(1);
  tft.drawString("WiFi", cx, 188);
}

void drawBatteryIcon() {
  if (!uiReady) {
    return;
  }

  const uint16_t bg = TFT_BLACK;
  uint16_t borderColor = TFT_DARKGREY;
  uint16_t fillColor = TFT_DARKGREY;
  const int x = 139;
  const int y = 153;
  const int w = 34;
  const int h = 18;
  const int tipX = x + w;
  const int tipY = y + 5;
  const int innerW = w - 6;
  const int innerH = h - 6;

  if (inaDataReady) {
    borderColor = TFT_WHITE;
    if (inaBatteryPercent >= 70) {
      fillColor = TFT_GREEN;
    } else if (inaBatteryPercent >= 30) {
      fillColor = TFT_YELLOW;
    } else {
      fillColor = TFT_RED;
    }
  }

  tft.fillRect(126, 148, 70, 54, bg);
  tft.drawRoundRect(x, y, w, h, 3, borderColor);
  tft.fillRect(tipX, tipY, 3, h - 10, borderColor);

  if (inaDataReady) {
    int fillW = (innerW * inaBatteryPercent) / 100;
    if (fillW < 0) {
      fillW = 0;
    }
    tft.fillRect(x + 3, y + 3, innerW, innerH, bg);
    if (fillW > 0) {
      tft.fillRect(x + 3, y + 3, fillW, innerH, fillColor);
    }
  }

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(inaDataReady ? borderColor : TFT_DARKGREY, bg);
  tft.setTextSize(1);
  if (inaDataReady) {
    char text[12];
    snprintf(text, sizeof(text), "%d%%", inaBatteryPercent);
    tft.drawString(text, x + 17, 186);
  } else {
    tft.drawString("BAT", x + 17, 186);
  }

  if (chargeConnected) {
    const uint16_t boltColor = TFT_ORANGE;
    const int bx = x + 42;
    const int by = y + 10;
    tft.fillTriangle(bx - 2, by - 6, bx + 2, by - 6, bx - 1, by - 1,
                     boltColor);
    tft.fillTriangle(bx, by - 1, bx + 4, by - 1, bx - 2, by + 7, boltColor);
  }
}

void drawChargeIndicator() {
  if (!uiReady) {
    return;
  }

  const uint16_t bg = TFT_BLACK;
  const uint16_t chargerColor = chargerInputPresent ? TFT_RED : TFT_DARKGREY;
  const uint16_t chargeColor = chargeConnected ? TFT_ORANGE : TFT_DARKGREY;

  tft.fillRect(28, 196, 184, 28, bg);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(chargerColor, bg);
  tft.setTextSize(1);
  tft.drawString(chargerInputPresent ? "CHARGER IN" : "CHARGER OUT", 120, 196);
  tft.setTextColor(chargeColor, bg);
  tft.drawString(chargeConnected ? "CHARGING" : "NOT CHARGING", 120, 208);
}

void drawInaIndicator() {
  if (!uiReady) {
    return;
  }

  const uint16_t bg = TFT_BLACK;
  const uint16_t color =
      inaDataReady ? TFT_CYAN : (inaI2cDetected ? TFT_YELLOW : TFT_DARKGREY);

  tft.fillRect(18, 112, 204, 42, bg);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(color, bg);
  tft.setTextSize(1);

  if (inaDataReady) {
    char line1[48];
    char line2[48];
    char line3[48];
    snprintf(line1, sizeof(line1), "%.2fV   %.0fmA", inaBusVoltage,
             inaCurrentMa);
    snprintf(line2, sizeof(line2), "Battery %d%%", inaBatteryPercent);
    if (chargeConnected) {
      const float delta = chargePeakVoltage - chargeStartVoltage;
      const unsigned long elapsedMin = (millis() - chargeSessionStartedAt) / 60000UL;
      snprintf(line3, sizeof(line3), "CHG %+0.3fV  %lumin", delta, elapsedMin);
    } else if (chargerInputPresent) {
      snprintf(line3, sizeof(line3), "CHARGER IN");
    } else {
      snprintf(line3, sizeof(line3), "CHARGER OUT");
    }
    tft.drawString(line1, 120, 116);
    tft.drawString(line2, 120, 128);
    tft.drawString(line3, 120, 140);
    return;
  }

  tft.drawString(inaStatusText, 120, 128);
}

void drawStatusIndicators() {
  drawWifiIndicator();
  drawBatteryIcon();
  drawChargeIndicator();
  drawInaIndicator();
}

void refreshStatusIndicators() {
  if (!uiReady) {
    return;
  }
  selectDisplay();
  drawStatusIndicators();
}

void showIdleScreen() {
  currentScreen = ScreenMode::Idle;
  if (!uiReady) {
    return;
  }
  selectDisplay();
  ui.showText("DISPLAY TEST", "Tap NFC card to verify UI and reader.",
              TFT_YELLOW, TFT_WHITE);
  drawStatusIndicators();
}

void showDetectedScreen(const String &uid) {
  currentScreen = ScreenMode::Detected;
  if (!uiReady) {
    return;
  }
  selectDisplay();
  ui.showText("NFC DETECTED", uid, TFT_GREEN, TFT_WHITE);
  drawStatusIndicators();
}

void showRemovedScreen() {
  currentScreen = ScreenMode::Removed;
  if (!uiReady) {
    return;
  }
  selectDisplay();
  ui.showText("CARD REMOVED", "NFC tag removed from reader.", TFT_ORANGE,
              TFT_WHITE);
  drawStatusIndicators();
}

void showWifiSuccessScreen() {
  currentScreen = ScreenMode::WifiSuccess;
  if (!uiReady) {
    return;
  }
  selectDisplay();
  ui.showText("WIFI SUCCESS!", "Wireless connection established.", TFT_GREEN,
              TFT_WHITE);
  drawStatusIndicators();
}

void renderCurrentScreen() {
  switch (currentScreen) {
  case ScreenMode::Idle:
    showIdleScreen();
    break;
  case ScreenMode::Detected:
    showDetectedScreen(currentUid);
    break;
  case ScreenMode::Removed:
    showRemovedScreen();
    break;
  case ScreenMode::WifiSuccess:
    showWifiSuccessScreen();
    break;
  }
}

String readUid() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", mfrc522.uid.uidByte[i]);
    uid += hex;
  }
  return uid;
}

uint8_t scanI2cDevice() {
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      return address;
    }
  }
  return 0;
}

int estimateBatteryPercent(float volts) {
  const float minV = 3.30f;
  const float maxV = 4.20f;
  if (volts <= minV) {
    return 0;
  }
  if (volts >= maxV) {
    return 100;
  }
  return static_cast<int>(((volts - minV) / (maxV - minV)) * 100.0f + 0.5f);
}

void updateInaStatusText() {
  if (inaIdentified) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "INA ID OK M:%04X D:%04X",
             inaManufacturerId, inaDeviceId);
    inaStatusText = buffer;
    return;
  }

  if (inaI2cDetected) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "INA I2C OK @ 0x%02X / ID FAIL",
             inaAddress);
    inaStatusText = buffer;
    return;
  }

  inaStatusText = "INA NOT FOUND";
}

bool inaRead16(uint8_t reg, uint16_t &value) {
  Wire.beginTransmission(inaAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(static_cast<int>(inaAddress), 2) != 2) {
    return false;
  }

  value = (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read();
  return true;
}

bool identifyInaDevice() {
  if (!inaI2cDetected) {
    return false;
  }

  if (!inaRead16(INA_REG_MANUFACTURER_ID, inaManufacturerId)) {
    return false;
  }

  if (!inaRead16(INA_REG_DEVICE_ID, inaDeviceId)) {
    return false;
  }

  inaIdentified = true;
  inaRead16(INA_REG_CONFIG, inaConfigRaw);
  Serial.printf("[INA] Manufacturer ID=0x%04X Device ID=0x%04X\r\n",
                inaManufacturerId, inaDeviceId);
  Serial.printf("[INA] CONFIG=0x%04X\r\n", inaConfigRaw);
  return true;
}

void sampleInaData() {
  if (!inaIdentified) {
    return;
  }

  if (!inaRead16(INA_REG_CONFIG, inaConfigRaw) ||
      !inaRead16(INA_REG_VBUS, inaRawBus)) {
    inaDataReady = false;
    updateInaStatusText();
    Serial.printf("[INA][WARN] Failed to read CONFIG/VBUS registers\r\n");
    return;
  }

  uint16_t rawShuntUnsigned = 0;
  if (!inaRead16(INA_REG_VSHUNT, rawShuntUnsigned)) {
    inaDataReady = false;
    updateInaStatusText();
    Serial.printf("[INA][WARN] Failed to read VSHUNT register\r\n");
    return;
  }

  inaRawShunt = static_cast<int16_t>(rawShuntUnsigned);
  inaBusVoltage = static_cast<float>(inaRawBus) * 0.00125f;
  inaShuntVoltageMv = static_cast<float>(inaRawShunt) * 0.0025f;
  inaSignedCurrentMa = -(inaShuntVoltageMv / 0.1f);
  inaCurrentMa = fabsf(inaSignedCurrentMa);
  inaBatteryPercentRaw = estimateBatteryPercent(inaBusVoltage);
  updateDisplayedBatteryPercent(inaBatteryPercentRaw);

  inaDataReady = true;
  updateInaStatusText();
  if (chargeConnected && inaBusVoltage > chargePeakVoltage) {
    chargePeakVoltage = inaBusVoltage;
  }

  Serial.printf("[INA] CFG=0x%04X RAW_VBUS=0x%04X RAW_VSHUNT=0x%04X\r\n",
                inaConfigRaw, inaRawBus, static_cast<uint16_t>(inaRawShunt));
  Serial.printf(
      "[INA] VBUS=%.3fV VSHUNT=%.3fmV CURRENT=%.1fmA SIGNED=%.1fmA BAT_RAW=%d%% BAT=%d%%\r\n",
      inaBusVoltage, inaShuntVoltageMv, inaCurrentMa, inaSignedCurrentMa,
      inaBatteryPercentRaw, inaBatteryPercent);

  applyChargingState(isBatteryActuallyCharging(), "INA current");
}

void logReaderVersion() {
  selectNfc();
  const byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("[NFC] RC522 VersionReg=0x%02X\r\n", version);
  if (version == 0x00 || version == 0xFF) {
    Serial.printf("[NFC][WARN] RC522 response looks invalid. Check wiring, power, and SPI pins.\r\n");
  } else {
    Serial.printf("[NFC] RC522 response looks valid.\r\n");
  }
  mfrc522.PCD_DumpVersionToSerial();
}

void updateWifiState() {
  const bool connectedNow = wifi.isConnected();
  if (!wifiStateKnown) {
    wifiConnected = connectedNow;
    wifiStateKnown = true;
    return;
  }

  if (connectedNow == wifiConnected) {
    return;
  }

  wifiConnected = connectedNow;
  if (wifiConnected) {
    Serial.printf("[WiFi] connected to %s, IP=%s\r\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    wifiBannerPending = true;
    wifiBannerStartedAt = millis();
    showWifiSuccessScreen();
  } else {
    Serial.printf("[WiFi] disconnected\r\n");
    renderCurrentScreen();
  }
}

void handleCardDetected(const String &uid) {
  currentUid = uid;
  cardPresent = true;
  missingReadCount = 0;
  lastReadFailed = false;

  Serial.printf("[NFC] Card presence detected.\r\n");
  Serial.printf("[NFC] Card UID Detected: %s\r\n", uid.c_str());
  showDetectedScreen(uid);

  selectNfc();
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void handleCardRemoved() {
  if (!cardPresent) {
    return;
  }

  Serial.printf("[NFC] Card removed: %s\r\n", currentUid.c_str());
  currentUid = "";
  cardPresent = false;
  missingReadCount = 0;
  lastReadFailed = false;

  showRemovedScreen();
  delay(800);
  showIdleScreen();
}
} // namespace

void setup() {
  esp_rom_printf("\r\n[EARLY] UI + NFC integration test start\r\n");
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000);

  pinMode(TFT_CS_PIN, OUTPUT);
  pinMode(NFC_SS_PIN, OUTPUT);
  pinMode(TFT_BL_PIN, OUTPUT);
  pinMode(CHARGE_PIN, INPUT_PULLUP);
  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(NFC_SS_PIN, HIGH);
  digitalWrite(TFT_BL_PIN, HIGH);

  Serial.printf(
      "[BOOT] Shared SPI TFT_CS=%d TFT_DC=%d TFT_BL=%d NFC_SS=%d NFC_RST=%d SCK=%d MISO=%d MOSI=%d\r\n",
      TFT_CS_PIN, TFT_DC_PIN, TFT_BL_PIN, NFC_SS_PIN, NFC_RST_PIN,
      SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

  beginSharedSpi();

  Serial.printf("[BOOT] Initializing UI\r\n");
  selectDisplay();
  ui.begin();
  ui.setBaseBackground(TFT_BLACK);
  uiReady = true;
  showIdleScreen();
  Serial.printf("[BOOT] UI ready\r\n");

  Serial.printf("[BOOT] Initializing I2C SDA=%d SCL=%d\r\n", I2C_SDA_PIN,
                I2C_SCL_PIN);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  inaAddress = scanI2cDevice();
  inaI2cDetected = (inaAddress != 0);
  if (inaI2cDetected) {
    Serial.printf("[INA] I2C device detected at 0x%02X\r\n", inaAddress);
    if (identifyInaDevice()) {
      sampleInaData();
    } else {
      Serial.printf("[INA][WARN] I2C device found but ID read failed\r\n");
      updateInaStatusText();
    }
  } else {
    Serial.printf("[INA][WARN] No I2C device detected on D4/D5\r\n");
    updateInaStatusText();
  }
  renderCurrentScreen();

  Serial.printf("[BOOT] Initializing UUID\r\n");
  uuid.begin();
  Serial.printf("[BOOT] UUID=%s\r\n", uuid.load().c_str());

  Serial.printf("[BOOT] Initializing WiFi\r\n");
  wifi.begin("SMHGA01 (" + uuid.load() + ")");
  wifiConnected = wifi.isConnected();
  wifiStateKnown = true;
  if (wifiConnected) {
    wifiBannerPending = true;
    wifiBannerStartedAt = millis();
    Serial.printf("[WiFi] connected to %s, IP=%s\r\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    showWifiSuccessScreen();
  } else {
    Serial.printf("[WiFi] not connected\r\n");
    showIdleScreen();
  }

  Serial.printf("[BOOT] Initializing charge sense on pin %d\r\n", CHARGE_PIN);
  pinMode(CHARGE_PIN, INPUT_PULLUP);
  chargerInputPresent = isChargerInputPresent(digitalRead(CHARGE_PIN));
  applyChargingState(isBatteryActuallyCharging(), "Initial");
  Serial.printf("[CHARGE] D7 input=%s\r\n",
                chargerInputPresent ? "HIGH" : "LOW");
  renderCurrentScreen();

  Serial.printf("[BOOT] Initializing NFC\r\n");
  selectNfc();
  mfrc522.PCD_Init();
  logReaderVersion();
  Serial.printf("[BOOT] NFC ready\r\n");
}

void loop() {
  const unsigned long now = millis();
  if (now - lastPollAt < POLL_INTERVAL_MS) {
    delay(5);
    return;
  }
  lastPollAt = now;

  updateWifiState();

  const bool chargerNow = isChargerInputPresent(digitalRead(CHARGE_PIN));
  if (chargerNow != chargerInputPresent) {
    chargerInputPresent = chargerNow;
    Serial.printf("[CHARGE] D7 input=%s\r\n",
                  chargerInputPresent ? "HIGH" : "LOW");
    applyChargingState(isBatteryActuallyCharging(), "D7 change");
  }

  if (wifiBannerPending && wifiConnected &&
      (now - wifiBannerStartedAt >= WIFI_BANNER_MS)) {
    wifiBannerPending = false;
    if (cardPresent) {
      showDetectedScreen(currentUid);
    } else {
      showIdleScreen();
    }
  }

  if (inaIdentified && (now - lastInaSampleAt >= INA_SAMPLE_MS)) {
    lastInaSampleAt = now;
    sampleInaData();
    refreshStatusIndicators();
  }

  selectNfc();
  const bool hasNewCard = mfrc522.PICC_IsNewCardPresent();
  const bool canReadCard = hasNewCard && mfrc522.PICC_ReadCardSerial();

  if (canReadCard) {
    const String uid = readUid();
    if (!cardPresent || uid != currentUid) {
      handleCardDetected(uid);
    } else {
      missingReadCount = 0;
    }
    return;
  }

  if (hasNewCard && !canReadCard && !lastReadFailed) {
    Serial.printf("[NFC][WARN] Card detected but UID read failed.\r\n");
    lastReadFailed = true;
  }

  if (cardPresent) {
    missingReadCount++;
    if (missingReadCount >= REMOVE_CONFIRM_COUNT) {
      handleCardRemoved();
    }
  }
}
