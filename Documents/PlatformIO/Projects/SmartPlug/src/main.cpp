#include "DigitalOut.h"
#include "FirmwareUpdater.h"
#include "dateTimeManagerX.h"
#include "digital_sensor.h"
#include "mqtt.h"
#include "uuid.h"
#include "wifiManager.h"
#include <ArduinoJson.h>
#include <esp_ota_ops.h>

// =============================================================
// [1] Firmware Version System (Single Source of Truth)
// =============================================================
#define FW_VER_MAJOR 0
#define FW_VER_MINOR 3
#define FW_VER_PATCH 2

#define FW_VERSION_CODE ((FW_VER_MAJOR * 100) + (FW_VER_MINOR * 10) + FW_VER_PATCH)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define FW_VERSION_STRING STR(FW_VER_MAJOR) "." STR(FW_VER_MINOR) "." STR(FW_VER_PATCH)

__attribute__((used))
const char FW_VERSION[] = "SMARTPLUG_FW_VERSION:" FW_VERSION_STRING;

// =============================================================
// [2] 하드웨어 및 서비스 설정
// =============================================================
const char* OTA_URL = "http://gym907-0001.iptime.org:3004";
const char* PROJ_ID = "41742426-d035-4756-ba17-f542ea75ab02";

WifiManager wifi = WifiManager({.ssid = "plugtest", .password = "fcfc50kc35"});
FirmwareUpdater updater = FirmwareUpdater(OTA_URL, PROJ_ID, FW_VERSION_CODE);

DateTimeManagerX dateTime = DateTimeManagerX();
Mqtt mqtt = Mqtt();
Uuid uuid = Uuid();

Sensor button = Sensor(20);
DigitalOut relay = DigitalOut(4);
DigitalOut led1 = DigitalOut(6, false); 
DigitalOut led2 = DigitalOut(1, false); 

String controlTopic = "smart_plug/{uuid}";

// 롤백 검증을 위한 상태 변수들
bool validated = false;
unsigned long bootTime = 0;
const unsigned long VALIDATION_TIMEOUT = 30000; // 30초 대기
bool isRollbackError = false;

String statusPayload(bool isOn) {
    return "{\"state\":\"" + String(isOn ? "on" : "off") + "\"}";
}

// =============================================================
// [3] Setup
// =============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n--- Boot Start ---");
    Serial.printf("FW Info: %s\n", FW_VERSION);
    Serial.printf("FW Code: %d\n", FW_VERSION_CODE);

    button.begin();
    led1.begin();
    led2.begin();
    relay.begin();

    // 현재 파티션의 OTA 상태 확인
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // 새 펌웨어로 부팅된 경우
            Serial.println("[SYSTEM] New firmware: Verification will start in loop.");
            led1.blink(100); 
            delay(2000);
        } 
        else if (ota_state == ESP_OTA_IMG_VALID) {
            // 이미 검증된 안정 버전인 경우: 파란 불빛으로 안심 시킴
            Serial.println("[SYSTEM] Stable firmware confirmed.");
            led1.on(); 
            delay(2000);
            led1.off();
        }
    }

    // 부팅 시그니처: 50ms 간격으로 3초 동안 점멸
    led2.blink(50);
    delay(3000);

    // WiFi 연결 및 LED RED 500ms 점등
    led2.blink(500);
    delay(2000);
    wifi.begin();

    // 펌웨어 업데이트 확인 (LED RED 250ms 점등)
    led2.blink(250);
    delay(2000);
    updater.performFirmwareUpdate(
        [](float progress) { Serial.printf("Update Progress: %.2f%%\n", progress * 100); },
        [](FirmwareUpdateResult results) { 
            Serial.printf("Update Result: %d\n", (int)results); 
        }
    );

    // UUID 및 MQTT 설정 (LED RED 100ms 점등)
    led2.blink(100);
    delay(2000);
    uuid.begin();
    controlTopic = "smart_plug/" + uuid.load();
    dateTime.begin();

    button.listen({{
        .event = LOW,
        .delayMs = 0,
        .action = []() {
            bool newState = !relay.isOn();
            newState ? relay.on() : relay.off();
            newState ? led1.on() : led1.off();
            mqtt.publish(controlTopic + "/status", statusPayload(newState));
        },
    }});

    // MQTT 시작 및 구독
    mqtt.begin("192.168.0.15", "plugtest", "fcfc50kc35");
    mqtt.subscribe((controlTopic + "/command").c_str());

    mqtt.onReceived([](String topic, String payload) {
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, payload)) return;
        if (!doc.containsKey("cmd")) return;

        String cmd = doc["cmd"].as<String>();
        cmd.toLowerCase();

        if (cmd == "on" || cmd == "off") {
            bool state = (cmd == "on");
            state ? relay.on() : relay.off();
            state ? led1.on() : led1.off();
            mqtt.publish(controlTopic + "/status", statusPayload(state));
        } else if (cmd == "status") {
            mqtt.publish(controlTopic + "/status", statusPayload(relay.isOn()));
        }
    });

    // Setup 로직이 끝난 시점부터 30초 롤백 타이머 시작
    bootTime = millis();
}

// =============================================================
// [4] Loop
// =============================================================
void loop() {
    unsigned long now = millis();
    static unsigned long lastPublish = 0;

    // [1] 현재 실행 중인 파티션의 실제 하드웨어 상태 확인
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
    // 상태를 정상적으로 가져왔고, 현재 '검증 대기(PENDING)' 상태인 경우에만 검증 로직 실행
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK && 
        ota_state == ESP_OTA_IMG_PENDING_VERIFY) {

        // A. 검증 성공 조건: WiFi와 MQTT가 모두 안정적으로 연결됨
        if (wifi.isconnected() && mqtt.connected()) {
            Serial.println("\n[SUCCESS] Verification Complete. Marking partition as VALID.");
            esp_ota_mark_app_valid_cancel_rollback(); 
            // 이 함수 호출 후 ota_state는 VALID로 변하므로 다음 루프부터 이 if문은 통과함
        } 
        // B. 검증 실패 조건: 30초가 경과할 때까지 PENDING 상태임
        else if (now - bootTime > VALIDATION_TIMEOUT) {
            Serial.println("\n[CRITICAL] Still PENDING after 30s. Marking as INVALID and Rebooting.");
            
            // 시각적 피드백 (고속 점멸)
            led2.blink(50);
            delay(3000);

            // 1. 하드웨어에 이 버전은 불량(INVALID)이라고 낙인찍고 롤백 명령
            esp_ota_mark_app_invalid_rollback_and_reboot();

            // 2. 만약 위 함수가 실패할 경우를 대비한 강제 재부팅 (수동 롤백)
            const esp_partition_t* next_partition = esp_ota_get_next_update_partition(NULL);
            if (next_partition != NULL) {
                esp_ota_set_boot_partition(next_partition);
            }
            ESP.restart();
        }
    }

    // 5초 주기로 현재 상태 발행 (상태 동기화)
    if (now - lastPublish > 5000) {
        lastPublish = now;
        if (mqtt.connected()) {
            mqtt.publish(controlTopic + "/status", statusPayload(relay.isOn()));
        }
    }

    delay(10); // Watchdog 방지
}