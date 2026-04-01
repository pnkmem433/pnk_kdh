#include "ArduinoJson.h"
#include <Arduino.h>

#include "FirmwareUpdater.h"
#include "bleManagerX.h"
#include "dateTimeManagerX.h"
#include "httpManageX.h"
#include "mqtt.h"
#include "storage.h"
#include "thermoHygroSensor.h"
#include "wifiManagerX.h"

// 서버 주소
#define SERVER_ADDRESS1 "http://gym907-0001.iptime.org:3002/data/addDataArray"
#define SERVER_ADDRESS2 "http://gym907-0001.iptime.org:3002/device/sensor-error"

// 와이파이 설정
WifiManagerX wifiManagerX = WifiManagerX({
    .isNeedsInternet = true,                                // 인터넷 필요 여부
    .timeout = 5000,                                        // 연결 타임아웃
    .rootWiFi = {.ssid = "PNKS_F", .password = "pnks1111"}, // 루트 인터넷
});

FirmwareUpdater updater =
    FirmwareUpdater("http://gym907-0001.iptime.org:3004",
                    "7a47c85b-7d7e-4b1e-a235-575a5d7ab130", 5);

// mqtt설정
Mqtt mqtt = Mqtt();

// 센서 클래스
#define BLE_NAME "PNKS-SmartFarm"
ThermoHygroSensor *thermoHygroSensor =
    new XyMd03ThermoHygroSensor(Serial1, 1, D5, D5);

HttpManageX httpManageX = HttpManageX(SERVER_ADDRESS1, SERVER_ADDRESS2); // http
Storage storage = Storage();                            // 저장공간
DateTimeManagerX dateTimeManagerX = DateTimeManagerX(); // 시간
BleManagerX bleManagerX = BleManagerX();                // ble

const long interval = 30 * 1000; // 전송주기(㎳)
WifiCredential wifiCredential = {.ssid = "",
                                 .password = ""}; // 임시 와이파이 저장

void setup() {
  delay(3000);                             // 3초 대기
  Serial.begin(115200);                    // 시리얼 모니터 시작
  Serial1.begin(9600, SERIAL_8N1, D6, D7); // 모드버스 준비
  storage.begin();                         // 스토리지 시작

  wifiManagerX.set(storage.loadWifis()); // 저장된 Wi-Fi 정보 불러오기
  wifiManagerX.connect();                // Wi-Fi 연결 시도

  updater.performFirmwareUpdate([](float progress) {},
                                [](FirmwareUpdateResult result) {
                                  delay(1000);
                                  if (result == FirmwareUpdateResult::SUCCESS) {
                                    storage.fileLock();
                                  }
                                });

  dateTimeManagerX.begin();   // 시간관리 시작
  thermoHygroSensor->begin(); // 온습도센서 시작

  Serial.println("Sensor test start.");
  Serial.println(
      "temperature: " + String(thermoHygroSensor->readTemperature()) + "℃");
  Serial.println("humidity: " + String(thermoHygroSensor->readHumidity()) +
                 "%");

  // 블루투스를 시작합니다.
  std::vector<String> bleUuids = storage.loadBleUuids();

  bleManagerX.begin({
      // 디바이스 이름, UUID, RX, TX 설정
      .bleCredential =
          {
              .name = BLE_NAME,
              .uuid = bleUuids[0],
              .rxChar = bleUuids[1],
              .txChar = bleUuids[2],
          },

      // 디바이스가 연결되었을때 와이파이 연결상태를 보냅니다.
      .onConnect =
          [&]() {
            if (wifiManagerX.getWifiStatus() == WifiStatus::connected) {
              bleManagerX.print("WIFI! SUCCESS.");
            } else {
              bleManagerX.print("WIFI! FAIL.");
            }
          },

      // 디바이스가 연결 해제되었을때 실행할 명령어입니다.
      .onDisconnect = [&]() {},

      // 데이터를 받을때 실행할 명령어
      .onDataReceive =
          [&](String data) {
            // !Reset명령어 수신 시, 와이파이 목록 삭제
            if (data.startsWith("!RESET")) {
              wifiManagerX.disconnect();
              wifiManagerX.reset();
              storage.resetWiFi();
              bleManagerX.print("OK! Reset WiFi.");
            }

            // !SSAN명령어 수신 시, 주변 와이파이 목록 반환
            else if (data.startsWith("!SCAN")) {
              bleManagerX.print("OK! START SCAN.");
              std::vector<String> detectedSsid = wifiManagerX.scanWifi();
              delay(10);
              bleManagerX.print("WIFI! LIST:" + String(detectedSsid.size()));
              for (String ssid : detectedSsid) {
                int length = ssid.length();
                int startIndex = 0;
                while (startIndex < length) {
                  int endIndex = min(startIndex + 20, length);
                  bleManagerX.print(ssid.substring(startIndex, endIndex));
                  startIndex = endIndex;
                }
              }
              bleManagerX.print("WIFI! END");
            }

            // !CREDENTIALS 명령어 수신 시, 저장된 와이파이 목록 보냄
            else if (data.startsWith("!CREDENTIALS")) {
              std::vector<String> detectedSsid =
                  wifiManagerX.getSavedCredentials();
              delay(10);
              bleManagerX.print("WIFI! LIST:" + String(detectedSsid.size()));
              for (String ssid : detectedSsid) {
                int length = ssid.length();
                int startIndex = 0;
                while (startIndex < length) {
                  int endIndex = min(startIndex + 20, length);
                  bleManagerX.print(ssid.substring(startIndex, endIndex));
                  startIndex = endIndex;
                }
              }

              bleManagerX.print("WIFI! END");
            }

            // !STATUS명령어 수신 시, 현제 와이파이 상태 보냄
            else if (data.startsWith("!STATUS")) {
              if (wifiManagerX.getWifiStatus() == WifiStatus::connected) {
                bleManagerX.print("WIFI! SUCCESS.");
              } else {
                bleManagerX.print("WIFI! FAIL.");
              }
            }

            // !S={SSID}명령어 수신 시, 추가할 ssid임시등록
            else if (data.startsWith("!S=")) {
              wifiCredential.ssid = data.substring(3);
              bleManagerX.print("OK! set SSID.");
            }

            // !S+={SSID}명령어 수신 시, 추가할 ssid에 추가등록
            else if (data.startsWith("!S+=")) {
              wifiCredential.ssid += data.substring(4);
              bleManagerX.print("OK! add SSID.");
              Serial.println(wifiCredential.ssid);
            }

            // !P={PASSWORD}명령어 수신 시, 추가할 password임시등록
            else if (data.startsWith("!P=")) {
              wifiCredential.password = data.substring(3);
              bleManagerX.print("OK! set Password.");
            }

            // !P+={PASSWORD}명령어 수신 시, 추사할 password에 추가등록
            else if (data.startsWith("!P+=")) {
              wifiCredential.password += data.substring(4);
              bleManagerX.print("OK! add Password.");
              Serial.println(wifiCredential.password);
            }
            //! CONNECT 명령어 수신 시 연결 시도
            else if (data.startsWith("!CONNECT")) {
              // 시도중인 메시지 전송
              bleManagerX.print("WIFI! CONNECTING.");

              // 연결중일때 잠시 기다리기(멀티스래드 오류 방지)
              while (wifiManagerX.getWifiStatus() == WifiStatus::connecting)
                ;

              // 연결 시도 후 결과 반환
              bool status = wifiManagerX.connect(wifiCredential);

              // 연결 성공시
              if (status) {
                // 와이파이 정보저장
                wifiManagerX.set({wifiCredential});
                storage.addWifi(wifiCredential);

                // 성공메시지 보내기
                bleManagerX.print("WIFI! SUCCESS.");
              }
              // 연결 실패시
              else {
                // 실패메시지 전달
                bleManagerX.print("WIFI! FAIL.");
              }

              // 내용 초기화
              wifiCredential = {.ssid = "", .password = ""};
            }

            // 뭔말일지 모르겟을때;
            else {
              bleManagerX.print("NO! unknown.");
            }
          },
  });

  BleCredential credential = bleManagerX.getBleCredential();
  Serial.println("bleCredential: {\"uuid\":\"" + credential.uuid +
                 "\", \"rx_char\": \"" + credential.rxChar +
                 "\", \"tx_char\":\"" + credential.txChar + "\"}");
  Serial.println("Name : " + credential.name);

  // uuid 설정
  httpManageX.setUuid(bleManagerX.getBleCredential().uuid);

  xTaskCreate(
      [](void *) {
        while (1) {
          if ((wifiManagerX.getWifiStatus() == WifiStatus::none) &&
              (!bleManagerX.isconnect())) {
            wifiManagerX.connect();
          }
          vTaskDelay(100);
        }
      },
      "wifi_connect", 10000, NULL, 2, NULL);

  xTaskCreate(
      [](void *) {
        int64_t lastDataSendTime = (esp_timer_get_time() / 1000ULL);
        while (1) {
          lastDataSendTime = (esp_timer_get_time() / 1000ULL) - 10 * 60 * 1000;
          while ((wifiManagerX.getWifiStatus() == WifiStatus::connected)) {
            int64_t currentMillis = (esp_timer_get_time() / 1000ULL);
            if (currentMillis - lastDataSendTime >= 10 * 60 * 1000) {
              lastDataSendTime += 10 * 60 * 1000;
              storage.getThermohygrometerData(
                  [](std::vector<ThermohygrometerData> dataList) {
                    vTaskDelay(1000);
                    return httpManageX.saveDataArray(dataList);
                  });
            }
            vTaskDelay(100);
          }
          while ((wifiManagerX.getWifiStatus() == WifiStatus::connected)) {
            vTaskDelay(100);
          }
          while ((wifiManagerX.getWifiStatus() != WifiStatus::connected)) {
            vTaskDelay(100);
          }
        }
      },
      "send_data", 10000, NULL, 1, NULL);

  xTaskCreate(
      [](void *) {
        int64_t lastDataSendTime = (esp_timer_get_time() / 1000ULL);
        while (1) {
          int64_t currentMillis = (esp_timer_get_time() / 1000ULL);

          if (currentMillis - lastDataSendTime >= interval) {
            lastDataSendTime += interval;

            DateTime now = dateTimeManagerX.now();
            if (now.isNotEmpty()) {
              float temp = thermoHygroSensor->readTemperature();
              float humi = thermoHygroSensor->readHumidity();

              if (!isnan(temp) && !isnan(humi)) {
                ThermohygrometerData data = {
                    .dateTime = now, .temp = temp, .humi = humi};
                Serial.print(now.toString() + "\t temp: " + String(temp) +
                             "℃\thumi: " + String(humi) +
                             "%\t trying to send...");

                bool isSuccess = false;

                if (wifiManagerX.getWifiStatus() == WifiStatus::connected) {
                  isSuccess = httpManageX.saveData(data);
                }

                if (!isSuccess) {
                  Serial.println("fail to send data to server. > saveData");
                  storage.addThermohygrometerData(
                      {.dateTime = now, .temp = temp, .humi = humi});
                } else {
                  Serial.println(
                      "success to send data to server. > not saveData");
                }
              } else {
                httpManageX.sendControlLog(2);
                storage.fileLock();
                ESP.restart();
              }
            }
          }

          vTaskDelay(10);
        }
      },
      "save_data", 10000, NULL, 1, NULL);

  xTaskCreate(
      [](void *) {
        String myUuid = bleManagerX.getBleCredential().uuid;

        // 0) 기준 앵커: 현재 시각을 '이전 5000ms 경계'로 내림
        int64_t nowMs = (esp_timer_get_time() / 1000ULL);
        int64_t baseAnchor = (nowMs / 5000) * 5000; // 0, 5000, 10000 ...

        // 1) 다음 슬롯 번호(1부터 시작): 5000, 10000, 15000...
        uint64_t nextSlot = ((nowMs - baseAnchor) / 5000) + 1;

        // 2) 지터 함수: -100 ~ 0ms
        auto gen_jitter = []() -> int32_t {
          return -((int32_t)(esp_random() % 501)); // -0 ~ -500
        };
        int32_t jitter = gen_jitter();
        int64_t nextDue = baseAnchor + (int64_t)nextSlot * 5000 + jitter;

        for (;;) {
          nowMs = (esp_timer_get_time() / 1000ULL);

          if (nowMs >= nextDue) {
            // --- 센서 읽기 & MQTT publish ---
            float temp = thermoHygroSensor->readTemperature();
            float humi = thermoHygroSensor->readHumidity();
            DateTime now = dateTimeManagerX.now();

            if (isnan(temp) || isnan(humi)) {
              httpManageX.sendControlLog(2);
              storage.fileLock();
              ESP.restart();
            }

            String jsonPayload = "{\"temp\": " + String(temp, 2) +
                                 ", \"humi\": " + String(humi, 2) +
                                 ", \"time\": \"" + now.toString() + "\"}";

            String topic = "smartFarm/heartbeat/" + myUuid;
            if (mqtt.connected()) {
              mqtt.publish(topic.c_str(),
                           jsonPayload.c_str()); // QoS0/retain=false 권장
            }

            // --- 다음 슬롯으로 정확히 점프(5000ms 간격 고정) + 새 지터 ---
            nextSlot++;
            jitter = gen_jitter(); // 매 슬롯마다 새로운 -100~0ms
            nextDue = baseAnchor + (int64_t)nextSlot * 5000 + jitter;

            // (만약 지연 등으로 슬롯을 여러 개 건너뛰었다면 보정)
            if (nowMs > nextDue + 5000) {
              nextSlot = ((nowMs - baseAnchor) / 5000) + 1;
              jitter = gen_jitter();
              nextDue = baseAnchor + (int64_t)nextSlot * 5000 + jitter;
            }
          }

          vTaskDelay(pdMS_TO_TICKS(10)); // CPU 점유 억제
        }
      },
      "send_mqtt", 4096, NULL, 1, NULL);

  mqtt.begin("gym907-0001.iptime.org", "pnks", "pnks1111"); // mqtt시작

  std::vector<String> topics = {
      "smartFarm/command/all/#",
      "smartFarm/command/" + bleManagerX.getBleCredential().uuid + "/#",
  };

  for (String topic : topics) {
    mqtt.subscribe(topic.c_str());
  }

  mqtt.onReceived([](String topic, String message) {
    String myUuid = bleManagerX.getBleCredential().uuid;
    DateTime now = dateTimeManagerX.now();
    String nowStr = now.toString();

    Serial.println("MQTT Received:");
    Serial.println(" Topic: " + topic);
    Serial.println(" Message: " + message);
    if (topic.startsWith("smartFarm/command/")) {
      String target = topic.substring(strlen("smartFarm/command/"));
      String cbTopic = "smartFarm/callback/" + myUuid;
      if (target == "all" || target == myUuid) {
        if (message.indexOf("\"action\": \"restart\"") >= 0) {
          String payload = String("{\"result\":\"success\",\"time\":\"") +
                           nowStr + String("\",\"action\":\"restart\"}");
          mqtt.publish(cbTopic.c_str(), payload.c_str());

          // 부가 처리
          httpManageX.sendControlLog(1);
          storage.fileLock();
          ESP.restart();
          return;
        } else if (message.indexOf("\"action\": \"version\"") >= 0) {
          String payload = "{\"result\":\"success\",\"time\":\"" + nowStr +
                           String("\",\"action\":\"version\",\"version\":") +
                           String(updater.getFirmwareVersionNumber()) + String("}");

          mqtt.publish(cbTopic.c_str(), payload.c_str());
          return;
        }

        else {
          String payload =
              String("{\"result\":\"fail\",\"time\":\"") + nowStr +
              String(
                  "\",\"action\":\"Unknown\",\"message\":\"unknown action\"}");
          mqtt.publish(cbTopic.c_str(), payload.c_str());
        }
      }
    }
  });

  Serial.println("Setup complete.");
}

void loop() {}