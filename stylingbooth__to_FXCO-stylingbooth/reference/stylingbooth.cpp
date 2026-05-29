/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - door 이벤트를 중심으로 RFID/LED/API를 오케스트레이션하는 메인 제어 루프.

  핵심 관찰 포인트:
  - 문 상태 기반 스캔 윈도우 시작/종료
  - scan percent publish와 tag 전송 타이밍
*/
/*
  모듈 목적:
  - 피팅룸 문 상태(MQTT 수신)를 기준으로 RFID 스캔 윈도우를 열고,
    스캔 결과를 Door API로 전송하며 LED 스트립 상태 명령을 브로커로 전달한다.

  핵심 상태전이:
  - door=open 수신: RFID 중단/태그 초기화, LED READY, DoorApi openDoor
  - door=closed 수신: RFID 시작, 스캔 시작 시각 저장, LED SCANNING, DoorApi closeDoor
  - 10초 스캔 만료: RFID 중단, 태그 필터링 후 전송, LED USING

  동시성 포인트:
  - MQTT 콜백(door 상태)과 DoorSensorTask(스캔 진행률/완료)가 lastCloseTime, reader 상태를 공유한다.
  - 현재 코드는 mutex 없이 단순 공유를 사용하므로, 설계상 "상태 전환이 빈번하지 않다"는 전제에 의존한다.
*/
#include <WiFi.h>

#include "doorApi.h"
#include "http.h"
#include "led.h"
#include "rfidReader.h"
#include "sensor.h"
#include "MqttLite.h"

#ifndef WIFI_SSID
#error "WIFI_SSID is not defined. Set -DWIFI_SSID=\\\"...\\\" in platformio.ini"
#endif
#ifndef WIFI_PASS
#error "WIFI_PASS is not defined. Set -DWIFI_PASS=\\\"...\\\" in platformio.ini"
#endif
#ifndef API_HOST
#error "API_HOST is not defined. Set -DAPI_HOST=\\\"...\\\" in platformio.ini"
#endif
#ifndef API_PORT
#error "API_PORT is not defined. Set -DAPI_PORT=<int> in platformio.ini"
#endif
#ifndef MQTT_HOST
#error "MQTT_HOST is not defined. Set -DMQTT_HOST=\\\"...\\\" in platformio.ini"
#endif
#ifndef MQTT_PORT
#error "MQTT_PORT is not defined. Set -DMQTT_PORT=<int> in platformio.ini"
#endif
#ifndef MQTT_USER
#error "MQTT_USER is not defined. Set -DMQTT_USER=\\\"...\\\" in platformio.ini"
#endif
#ifndef MQTT_PASS
#error "MQTT_PASS is not defined. Set -DMQTT_PASS=\\\"...\\\" in platformio.ini"
#endif

WiFiClient wifiClient;

Http http = Http(&wifiClient);

DoorApi door = DoorApi(&http, API_HOST, API_PORT);

Sensor doorSensor = Sensor(D1);
Sensor resetButton = Sensor(D3);
LED led = LED(D4);

RfidReader *reader =
    new FonkanFF704RfidReader(Serial1, D7, D6, 115200);

Mqtt mqtt = Mqtt({
    .host = MQTT_HOST,
    .port = MQTT_PORT,
    .user = MQTT_USER,
    .password = MQTT_PASS,
    .useTls = false,
    .caCert = "",
    .autoReconnect = true,
});

uint64_t lastCloseTime = 0;
uint64_t readingTime = 10ULL * 1000ULL * 1000ULL; // 10초(마이크로초)

/*
  connectWiFi()
  - WiFi 연결 완료까지 대기한다.
  - 실패 시 무기한 대기할 수 있으므로 운영 환경에서 SSID/암호 오타를 우선 점검해야 한다.
*/
void connectWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  WiFi.setAutoReconnect(true);
}

void setup()
{
  Serial.begin(115200);
  delay(3000);

  led.begin();
  led.blink(500);

  reader->onRead([](String tag)
                 {
                   // 실시간 수신 로그:
                   // - 디버깅 시 리더 수신 여부를 즉시 확인
                   // - 운영 배포 시 로그량이 많아질 수 있어 필요 시 축소 가능
                   Serial.printf("Read tag: %s\n", tag.c_str());
                 });

  // 초기에는 한번 start 후 stop 해두고, 실제 시작은 door closed 이벤트에서 수행
  reader->startRead();

  connectWiFi();

  mqtt.begin();
  mqtt.connect();

  mqtt.onReceived({
      .topic = "fittingroom/door/9169A97A-96DE-4FFF-9167-A2B398C6C900/door",
      .qos = 0,
      .callback =
          [](const MqttMessage &msg)
      {
        // 입력 계약:
        // - msg.raw에 door 상태 문자열(open/closed)이 들어온다고 가정
        // - 공백/대소문자 편차를 정규화해 비교 오류를 방지
        String payload = msg.raw;
        payload.trim();
        payload.toLowerCase();

        if (payload == "open")
        {
          // 상태전이: closed/scanning -> open/ready
          // 1) RFID 중지 및 임시 태그 폐기
          // 2) 스캔 타이머 초기화(lastCloseTime=0)
          // 3) LED 장치 READY 명령 publish
          // 4) Door API open 상태 반영
          reader->stopRead();
          reader->clearTags();
          lastCloseTime = 0;

          mqtt.publish({
              .topic = "fittingroom/led_strip/9642C3BA-FC4A-4B07-A0E0-9153D323EC06/command/mode",
              .payload = "READY",
              .qos = 0,
              .retain = false,
              .timeoutMs = 2000,
          });

          door.openDoor();
        }
        else if (payload == "closed")
        {
          // 상태전이: open/ready -> closed/scanning
          // 1) RFID 시작
          // 2) 스캔 시작 시각 기록
          // 3) LED 장치 SCANNING 명령 publish
          // 4) Door API closed 상태 반영
          reader->startRead();
          lastCloseTime = esp_timer_get_time();

          mqtt.publish({
              .topic = "fittingroom/led_strip/9642C3BA-FC4A-4B07-A0E0-9153D323EC06/command/mode",
              .payload = "SCANNING",
              .qos = 0,
              .retain = false,
              .timeoutMs = 2000,
          });

          door.closeDoor();
        }
      },
  });

  http.begin();
  door.begin();
  reader->begin();
  reader->stopRead();

  door.setFittingRoomId(1);

  led.on();

  // DoorSensorTask
  // - 역할: 문 닫힘 이후 스캔 진행률 publish, 스캔 만료 시 태그 필터링/전송.
  // - 주기: 100ms(vTaskDelay)
  // - 100ms를 쓰는 이유: 1초 단위 진행률 표시의 분해능 확보 + MQTT 트래픽 과도 증가 방지.
  xTaskCreate(
      [](void *param)
      {
        // 태스크 계약:
        // - 입력: 없음(공유 상태 lastCloseTime, reader tags 사용)
        // - 출력: scan percent/mode publish, tag 전송
        // - 주의: MQTT 콜백과 공유 변수 접근이 교차하므로 상태전이 순서 유지가 중요
        static int lastPercent = -1;

        while (1)
        {
          if (lastCloseTime != 0)
          {
            uint64_t now = esp_timer_get_time();
            uint64_t elapsed = now - lastCloseTime;

            if (elapsed >= readingTime)
            {
              // 스캔 윈도우 종료 처리:
              // - SCANNING 종료 -> USING 전환
              // - 태그 필터 후 Door API 전송
              reader->stopRead();
              led.blink(100);

              mqtt.publish({
                  .topic = "fittingroom/led_strip/9642C3BA-FC4A-4B07-A0E0-9153D323EC06/command/mode",
                  .payload = "USING",
                  .qos = 0,
                  .retain = false,
                  .timeoutMs = 2000,
              });

              std::map<String, int> tags = reader->getTags();
              std::vector<String> tagList;

              for (const auto &tag : tags)
              {
                // 노이즈 필터: 동일 태그가 5회 이상 읽힌 경우만 유효 태그로 간주
                // 이유: 순간 오검출을 줄이고 실제 착의 품목만 전송하기 위함.
                if (tag.second >= 5)
                {
                  // substring(5,13) 규약:
                  // - 리더 원문 프레임에서 서버가 요구하는 코드 구간만 추출
                  tagList.push_back(tag.first.substring(5, 13));
                }
              }

              if (!tagList.empty())
              {
                door.sendTags(tagList);
              }

              reader->clearTags();
              led.on();

              lastCloseTime = 0;
              lastPercent = -1;
            }
            else
            {
              // 마지막 1초는 정리 구간으로 보고 퍼센트 계산 분모에서 제외
              uint64_t scanTime = readingTime - (1ULL * 1000ULL * 1000ULL);

              int percent = (elapsed * 100) / scanTime;
              if (percent > 100)
                percent = 100;

              // 변화가 있을 때만 publish해 브로커 부하를 줄임
              if (percent != lastPercent)
              {
                lastPercent = percent;

                // 진행률 publish:
                // - UI 갱신용 실시간 피드백
                // - 변화량 있을 때만 전송해 메시지 폭주를 방지
                mqtt.publish({
                    .topic = "fittingroom/led_strip/9642C3BA-FC4A-4B07-A0E0-9153D323EC06/command/scan/percent",
                    .payload = String(percent),
                    .qos = 0,
                    .retain = false,
                    .timeoutMs = 2000,
                });
              }
            }
          }

          vTaskDelay(100 / portTICK_PERIOD_MS);
        }
      },
      "DoorSensorTask",
      4096,
      NULL,
      1,
      NULL);
}

void loop()
{
  delay(10);
}


