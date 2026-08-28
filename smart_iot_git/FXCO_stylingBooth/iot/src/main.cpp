#include "FirmwareUpdater.h"
#include "doorSensor.h"
#include "hexAsciiConverter.h"
#include "httpManageX.h"
#include "led.h"
#include "pir.h"
#include "rfidReader.h"

DoorSensor fittingroomDoor = DoorSensor(D0);
DoorSensor showroomDoor = DoorSensor(D1);
DoorSensor resetButton = DoorSensor(D3);

Pir pirSensor = Pir(D2);

Led led = Led(D4);

RfidReader *reader = new FonkanFF704RfidReader(Serial1, D7, D6, 115200);

HttpManageX http = HttpManageX("192.168.1.100", 3000);

FirmwareUpdater updater = FirmwareUpdater(
    "gym907-0001.iptime.org", 3004, "0a54bc0c-630b-4235-9657-2bdfef0b23ab", 15);

void setup() {
  Serial.begin(115200);
  http.initMutex();
  reader->begin();
  led.begin();
  led.blink(500);

  UIPEthernet.init(D5);
  byte mac[6];
  esp_read_mac(mac, ESP_MAC_ETH);

  if (UIPEthernet.begin(mac) == 0) {
    Serial.println("⚠️ DHCP 실패 - Ethernet 연결 안 됨");

    led.off();
    delay(1000);

    ESP.restart(); // DHCP 실패 시 ESP 재시작
  } else {
    Serial.println("✅ Ethernet 연결됨!");
    Serial.print("IP 주소: ");
    Serial.println(UIPEthernet.localIP());
  }

  delay(2500);

  led.blink(250);

  updater.performFirmwareUpdate(
      [](float progress) {
        Serial.println("update progress: " + String(progress * 100) + "%");
      },
      [](FirmwareUpdateResult result) {
        if (result == FirmwareUpdateResult::SUCCESS) {
          Serial.println("successed to update firmware!");
        } else {
          Serial.println("failed to update firmware: " +
                         String(static_cast<int>(result)));
        }
      });

  led.blink(100);

  // HTTP 세션 설정
  int session = http.setSession();
  int loadingTime = http.loadLoadingTime();

  if (session <= 0 || loadingTime <= 0) {
    Serial.println("⚠️ 세션 설정 실패");
    led.off();
    delay(1000);
    ESP.restart(); // 세션 설정 실패 시 ESP 재시작
  } else {
    Serial.printf("✅ 세션 설정됨: %d, 로딩 시간: %d초\n", session,
                  loadingTime);
  }

  pirSensor.begin();
  led.on();

  fittingroomDoor.begin({
      {
          .event = DoorEvent::Closed,
          .delayMs = NO_DELAY,
          .action =
              []() {
                Serial.println("[event]\tFitting room door closed");
                led.blink(50);
                reader->startRead();
                http.closeFittingRoomDoor();
                pirSensor.clear();
                led.on();
              },
      },
      {.event = DoorEvent::Opened,
       .delayMs = NO_DELAY,
       .action =
           []() {
             Serial.println("[event]\tFitting room door opened");
             led.blink(50);
             reader->stopRead();
             reader->clearTags();
             http.openFittingRoomDoor();
             led.on();
           }},
      {.event = DoorEvent::Closed,
       .delayMs = 6500,
       .action =
           []() {
             Serial.println(
                 "[event]\tCheck if there are people in the fitting room.");
             led.blink(50);
             http.checkPeopleInsideFittingRoom(pirSensor.isDetected());
             led.on();
           }},
      {.event = DoorEvent::Closed,
       .delayMs = loadingTime * 1000,
       .action =
           []() {
             Serial.println("[event]\tRFID reading completed");
             reader->stopRead();
             led.blink(50);
             std::map<String, int> tags = reader->getTags();

             for (auto &tag : tags) {
               Serial.println("Tag: " + tag.first +
                              ", Count: " + String(tag.second));
             }

             if (!tags.empty()) {
               int tagCount = 0;
               String tagName;
               for (auto tag : tags) {
                 String asciiTag = HexAsciiConverter::convert(tag.first);
                 if (!asciiTag.isEmpty() && asciiTag.length() == 7) {
                   if (tag.second > tagCount) {
                     tagCount = tag.second;
                     tagName = asciiTag;
                   }
                 }
               }
               if (!tagName.isEmpty()) {
                 http.sendTag(tagName);
               }
             }
             reader->clearTags();
             led.on();
           }},
  });

  showroomDoor.begin({
      {.event = DoorEvent::Closed,
       .delayMs = NO_DELAY,
       .action =
           []() {
             Serial.println("[event]\tShowroom door closed");
             led.blink(50);
             http.closeShowroomDoor();
             led.on();
           }},
      {.event = DoorEvent::Opened,
       .delayMs = NO_DELAY,
       .action =
           []() {
             Serial.println("[event]\tShowroom door opened");
             led.blink(50);
             http.openShowroomDoor();
             led.on();
           }},
  });

  resetButton.begin({
      {.event = DoorEvent::Closed,
       .delayMs = NO_DELAY,
       .action = []() { led.blink(50); }},
      {.event = DoorEvent::Opened,
       .delayMs = NO_DELAY,
       .action = []() { led.on(); }},
      {.event = DoorEvent::Closed,
       .delayMs = 3000,
       .action = []() { ESP.restart(); }},
  });
}

void loop() {
  if (http.safeLinkStatus() == EthernetLinkStatus::LinkOFF) {
    Serial.println("⚠️ Ethernet link is OFF, restarting...");
    led.off();
    delay(2000);
    ESP.restart();
  }

  delay(4800);
}