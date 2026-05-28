# D3/D4 Swapped Main Restore

`src/main.cpp`를 원래 코드로 되돌린 뒤, `D3`와 `D4` 역할만 다시 바꿔야 할 때 참고하는 복원용 문서입니다.

## 변경 요약

원래 코드:

```cpp
  - D3  : LED 출력
  - D4  : 터치 버튼 입력 (Sensor, 기능 TBD)

LED    led      = LED(D3);
Sensor touchBtn = Sensor(D4);  // 터치 입력 (기능 TBD)
```

교체 코드:

```cpp
  - D3  : 터치 버튼 입력 (Sensor, 기능 TBD)
  - D4  : LED 출력

Sensor touchBtn = Sensor(D3);  // 터치 입력 (기능 TBD)
LED    led      = LED(D4);
```

## 복원용 전체 코드

아래 코드는 사용자가 보낸 원래 `main.cpp`를 기준으로, `D3`와 `D4`만 서로 바꿔 놓은 버전입니다.

```cpp
/*
  파일 역할:
  - 피팅룸/쇼룸 문 이벤트를 중심으로 RFID/LED/HTTP를 오케스트레이션하는 메인 제어 루프.

  핀맵:
  - D1  : 피팅룸 문 센서 (DoorSensor)
  - D2  : 쇼룸 문 센서 (DoorSensor)
  - D3  : 터치 버튼 입력 (Sensor, 기능 TBD)
  - D4  : LED 출력
  - D5  : Ethernet CS (LAN)
  - D6  : RFID RX (Serial1)
  - D7  : RFID TX (Serial1)
  - D8  : Ethernet SCK
  - D9  : Ethernet SO (MISO)
  - D10 : Ethernet SI (MOSI)

  핵심 상태전이:
  - fittingroomDoor Closed (즉시)         : RFID 시작, closeFittingRoomDoor
  - fittingroomDoor Opened (즉시)         : RFID 중지/태그 초기화, openFittingRoomDoor
  - fittingroomDoor Closed (loadingTime 후): RFID 중지, 태그 필터링 후 전송
  - showroomDoor Closed/Opened (즉시)     : close/openShowroomDoor
*/
#include "FirmwareUpdater.h"
#include "doorSensor.h"
#include "hexAsciiConverter.h"
#include "httpManageX.h"
#include "lan.h"
#include "led.h"
#include "rfidReader.h"
#include "sensor.h"

#ifndef API_HOST
#error "API_HOST is not defined. Set -DAPI_HOST=\\\"...\\\" in platformio.ini"
#endif
#ifndef API_PORT
#error "API_PORT is not defined. Set -DAPI_PORT=<int> in platformio.ini"
#endif
#ifndef FIRMWARE_HOST
#error "FIRMWARE_HOST is not defined. Set -DFIRMWARE_HOST=\\\"...\\\" in platformio.ini"
#endif
#ifndef FIRMWARE_PORT
#error "FIRMWARE_PORT is not defined. Set -DFIRMWARE_PORT=<int> in platformio.ini"
#endif
#ifndef FIRMWARE_ID
#error "FIRMWARE_ID is not defined. Set -DFIRMWARE_ID=\\\"...\\\" in platformio.ini"
#endif

// --- 핀 배치 ---
DoorSensor fittingroomDoor = DoorSensor(D1, INPUT_PULLUP);    // 액티브 LOW
DoorSensor showroomDoor    = DoorSensor(D2, INPUT_PULLDOWN);  // 액티브 HIGH

LED    led      = LED(D4);
Sensor touchBtn = Sensor(D3);  // 터치 입력 (기능 TBD)

LAN lan = LAN(D5);

RfidReader *reader = new FonkanFF704RfidReader(Serial1, D7, D6, 115200);

HttpManageX    http    = HttpManageX(API_HOST, API_PORT);
FirmwareUpdater updater = FirmwareUpdater(FIRMWARE_HOST, FIRMWARE_PORT, FIRMWARE_ID, 15);

// ---------------------------------------------------------------------------

void setup()
{
  delay(5000);

  Serial.begin(38400);
  Serial.println("1");

  HttpManageX::initMutex();
  Serial.println("2");

  reader->begin();
  reader->onRead([](String tag) {
    Serial.printf("[rfid]\tRaw tag: %s\n", tag.c_str());
  });
  Serial.println("3");

  led.begin();
  Serial.println("4");

  led.blink(500);
  delay(2500);
  Serial.println("5");

  if (!lan.begin())
  {
    Serial.println("⚠️ Ethernet DHCP 실패");
    led.off();
    delay(1000);
    ESP.restart();
  }

  Serial.println("Ethernet connected!");
  Serial.print("IP: ");
  Serial.println(lan.getLocalIP());
  Serial.println("6");

  delay(2500);

  led.blink(250);

  // DHCP 할당 후 쌓인 패킷 비우기
  for(int i = 0; i < 3; i++) {
    http.safeLinkStatus();
    delay(100);
  }

  updater.performFirmwareUpdate(
      [](float progress)
      {
        Serial.println("update progress: " + String(progress * 100) + "%");
      },
      [](FirmwareUpdateResult result)
      {
        if (result == FirmwareUpdateResult::SUCCESS)
          Serial.println("successed to update firmware!");
        else if (result == FirmwareUpdateResult::NO_UPDATE_NEEDED)
          Serial.println("firmware is up-to-date. (no update needed)");
        else
          Serial.println("failed to update firmware: " +
                         String(static_cast<int>(result)));
      });

  led.blink(100);

  // 버퍼에 남아있는 찌꺼기 패킷(TCP 종료 패킷 등)을 비워주기 위해 호출
  for(int i = 0; i < 5; i++) {
    http.safeLinkStatus();
    delay(100);
  }

  // 세션 및 로딩 시간 설정
  int session     = http.setSession();
  int loadingTime = http.loadLoadingTime();

  if (session <= 0 || loadingTime <= 0)
  {
    Serial.println("⚠️ 세션 설정 실패");
    led.off();
    delay(1000);
    ESP.restart();
  }
  else
  {
    Serial.printf("✅ 세션 설정됨: %d, 로딩 시간: %d초\n", session, loadingTime);
  }

  led.on();

  // --- 피팅룸 문 이벤트 ---
  fittingroomDoor.begin({
      {
          .event   = DoorEvent::Closed,
          .delayMs = NO_DELAY,
          .action  = []()
          {
            Serial.println("[event]\tFitting room door closed");
            led.blink(50);
            reader->startRead();
            http.closeFittingRoomDoor();
            led.on();
          },
      },
      {
          .event   = DoorEvent::Opened,
          .delayMs = NO_DELAY,
          .action  = []()
          {
            Serial.println("[event]\tFitting room door opened");
            led.blink(50);
            reader->stopRead();
            reader->clearTags();
            http.openFittingRoomDoor();
            led.on();
          },
      },
      {
          .event   = DoorEvent::Closed,
          .delayMs = loadingTime * 1000,
          .action  = []()
          {
            Serial.println("[event]\tRFID reading completed");
            reader->stopRead();
            led.blink(50);

            std::map<String, int> tags = reader->getTags();

            for (auto &tag : tags)
              Serial.println("Tag: " + tag.first +
                             ", Count: " + String(tag.second));

            if (!tags.empty())
            {
              int    tagCount = 0;
              String tagName;

              for (auto tag : tags)
              {
                String asciiTag = HexAsciiConverter::convert(tag.first);
                if (!asciiTag.isEmpty() && asciiTag.length() == 7)
                {
                  if (tag.second > tagCount)
                  {
                    tagCount = tag.second;
                    tagName  = asciiTag;
                  }
                }
              }

              if (!tagName.isEmpty())
                http.sendTag(tagName);
            }

            reader->clearTags();
            led.on();
          },
      },
  });

  // --- 쇼룸 문 이벤트 ---
  showroomDoor.begin({
      {
          .event   = DoorEvent::Closed,
          .delayMs = NO_DELAY,
          .action  = []()
          {
            Serial.println("[event]\tShowroom door closed");
            led.blink(50);
            http.closeShowroomDoor();
            led.on();
          },
      },
      {
          .event   = DoorEvent::Opened,
          .delayMs = NO_DELAY,
          .action  = []()
          {
            Serial.println("[event]\tShowroom door opened");
            led.blink(50);
            http.openShowroomDoor();
            led.on();
          },
      },
  });
}

void loop()
{
  if (http.safeLinkStatus() == EthernetLinkStatus::LinkOFF)
  {
    Serial.println("⚠️ Ethernet link is OFF, restarting...");
    led.off();
    delay(2000);
    ESP.restart();
  }

  delay(4800);
}
```
