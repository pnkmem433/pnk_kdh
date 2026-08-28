#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

#warning **** user_config_override.h: Using Smart Plug Settings ****

// Force SECTION1 defaults to be reloaded on first boot after reflashing.
#undef CFG_HOLDER
#define CFG_HOLDER       4631

// Keep logs readable but avoid excessive UART traffic on the deployed smart plug.
#undef SERIAL_LOG_LEVEL
#define SERIAL_LOG_LEVEL LOG_LEVEL_INFO

#undef WEB_LOG_LEVEL
#define WEB_LOG_LEVEL    LOG_LEVEL_INFO

#undef WEB_SERVER
#define WEB_SERVER        2

#ifdef WEB_USERNAME
#undef WEB_USERNAME
#endif
#define WEB_USERNAME      "pnkmem433"

#undef WEB_PASSWORD
#define WEB_PASSWORD      "pnks1111"

// Default network values for the custom smart-plug test image.
#undef STA_SSID1
#ifdef FIRMWARE_OTA_BRIDGE
#define STA_SSID1         ""
#else
#define STA_SSID1         "CC-Retail"
#endif

#undef STA_PASS1
#define STA_PASS1         "pnks1111"

#undef WIFI_CONFIG_TOOL
#define WIFI_CONFIG_TOOL  WIFI_MANAGER

#ifdef FIRMWARE_PARTITION_BRIDGE
#ifndef USE_BERRY
#define USE_BERRY
#endif

#ifndef USE_WEBCLIENT_HTTPS
#define USE_WEBCLIENT_HTTPS
#endif

#ifndef USE_UFILESYS
#define USE_UFILESYS
#endif

#ifndef USE_EXTENSION_MANAGER
#define USE_EXTENSION_MANAGER
#endif

#ifdef USE_LIGHT
#undef USE_LIGHT
#endif

#ifdef USE_WS2812
#undef USE_WS2812
#endif

#ifdef USE_LIGHT_ARTNET
#undef USE_LIGHT_ARTNET
#endif

#ifdef USE_BERRY_ANIMATION
#undef USE_BERRY_ANIMATION
#endif
#endif

#undef MQTT_HOST
#define MQTT_HOST         "api.pnkslab.com"

#undef MQTT_PORT
#define MQTT_PORT         1883

#undef MQTT_USER
#define MQTT_USER         "pnks"

#undef MQTT_PASS
#define MQTT_PASS         "pnks1111"

#ifdef FIRMWARE_MQTT_OTA_MINIMAL
#undef WEB_SERVER
#define WEB_SERVER        2

#undef WIFI_CONFIG_TOOL
#define WIFI_CONFIG_TOOL  WIFI_MANAGER

#ifdef WEBSERVER_ADVERTISE
#undef WEBSERVER_ADVERTISE
#endif

#ifdef USE_WEB_STATUS_LINE
#undef USE_WEB_STATUS_LINE
#endif

#ifdef USE_ARDUINO_OTA
#undef USE_ARDUINO_OTA
#endif

#ifdef USE_BERRY
#undef USE_BERRY
#endif

#ifdef USE_WEBCLIENT_HTTPS
#undef USE_WEBCLIENT_HTTPS
#endif

#ifdef USE_TLS
#undef USE_TLS
#endif

#ifdef USE_UFILESYS
#undef USE_UFILESYS
#endif

#ifdef USE_AUTOCONF
#undef USE_AUTOCONF
#endif

#ifdef USE_DISCOVERY
#undef USE_DISCOVERY
#endif

#ifdef USE_TASMOTA_DISCOVERY
#undef USE_TASMOTA_DISCOVERY
#endif

#ifdef USE_HOME_ASSISTANT
#undef USE_HOME_ASSISTANT
#endif

#ifdef USE_DOMOTICZ
#undef USE_DOMOTICZ
#endif

#ifdef USE_KNX
#undef USE_KNX
#endif

#ifdef USE_DALI
#undef USE_DALI
#endif

#ifdef USE_INFLUXDB
#undef USE_INFLUXDB
#endif

#ifdef USE_PROMETHEUS
#undef USE_PROMETHEUS
#endif

#ifdef USE_TELEGRAM
#undef USE_TELEGRAM
#endif

#ifdef USE_UPNP
#undef USE_UPNP
#endif

#ifdef USE_HUE
#undef USE_HUE
#endif

#ifdef USE_WEMO
#undef USE_WEMO
#endif

#ifdef USE_WEBSEND_RESPONSE
#undef USE_WEBSEND_RESPONSE
#endif

#ifdef USE_EXTENSION_MANAGER
#undef USE_EXTENSION_MANAGER
#endif

#ifdef USE_RULES
#undef USE_RULES
#endif

#ifdef USE_SCRIPT
#undef USE_SCRIPT
#endif

#ifdef USE_TIMERS
#undef USE_TIMERS
#endif

#ifdef USE_TIMERS_WEB
#undef USE_TIMERS_WEB
#endif

#ifdef USE_SUNRISE
#undef USE_SUNRISE
#endif

#ifdef USE_EXPRESSION
#undef USE_EXPRESSION
#endif

#ifdef USE_PING
#undef USE_PING
#endif

#ifdef USE_MDNS
#undef USE_MDNS
#endif

#ifdef USE_SYSLOG
#undef USE_SYSLOG
#endif

#ifdef USE_ENHANCED_GUI_WIFI_SCAN
#undef USE_ENHANCED_GUI_WIFI_SCAN
#endif

#ifdef USE_EMULATION
#undef USE_EMULATION
#endif

#ifdef USE_EMULATION_HUE
#undef USE_EMULATION_HUE
#endif

#ifdef USE_EMULATION_WEMO
#undef USE_EMULATION_WEMO
#endif

#ifdef USE_DISCOVERY
#undef USE_DISCOVERY
#endif

#ifdef USE_UNISHOX_COMPRESSION
#undef USE_UNISHOX_COMPRESSION
#endif

#ifdef USE_DISPLAY
#undef USE_DISPLAY
#endif

#ifdef USE_LIGHT
#undef USE_LIGHT
#endif

#ifdef USE_WS2812
#undef USE_WS2812
#endif

#ifdef USE_LIGHT_ARTNET
#undef USE_LIGHT_ARTNET
#endif

#ifdef USE_BERRY_ANIMATION
#undef USE_BERRY_ANIMATION
#endif

#ifdef USE_DOMOTICZ
#undef USE_DOMOTICZ
#endif

#ifdef USE_HOME_ASSISTANT
#undef USE_HOME_ASSISTANT
#endif

#ifdef USE_TASMOTA_DISCOVERY
#undef USE_TASMOTA_DISCOVERY
#endif

#ifdef USE_TELEGRAM
#undef USE_TELEGRAM
#endif

#ifdef USE_INFLUXDB
#undef USE_INFLUXDB
#endif

#ifdef USE_KNX
#undef USE_KNX
#endif

#ifdef USE_DALI
#undef USE_DALI
#endif

#ifdef USE_PROMETHEUS
#undef USE_PROMETHEUS
#endif

#ifdef USE_LIGHT_PALETTE
#undef USE_LIGHT_PALETTE
#endif

#ifdef USE_I2C
#undef USE_I2C
#endif

#ifdef USE_SPI
#undef USE_SPI
#endif

#ifdef USE_DS18x20
#undef USE_DS18x20
#endif

#ifdef USE_DS18X20
#undef USE_DS18X20
#endif

#ifdef USE_DS18X20_LEGACY
#undef USE_DS18X20_LEGACY
#endif

#ifdef USE_DHT
#undef USE_DHT
#endif

#ifdef USE_ENERGY_SENSOR
#undef USE_ENERGY_SENSOR
#endif

#ifdef USE_ADC_VCC
#undef USE_ADC_VCC
#endif

#ifdef USE_COUNTER
#undef USE_COUNTER
#endif

#ifdef USE_BMP
#undef USE_BMP
#endif

#ifdef USE_BME680
#undef USE_BME680
#endif

#ifdef USE_BH1750
#undef USE_BH1750
#endif

#ifdef USE_SHT
#undef USE_SHT
#endif

#ifdef USE_HTU
#undef USE_HTU
#endif

#ifdef USE_HX711
#undef USE_HX711
#endif

#ifdef USE_SERIAL_BRIDGE
#undef USE_SERIAL_BRIDGE
#endif

#ifdef USE_MODBUS_BRIDGE
#undef USE_MODBUS_BRIDGE
#endif

#ifdef USE_TCP_BRIDGE
#undef USE_TCP_BRIDGE
#endif

#ifdef USE_ZIGBEE
#undef USE_ZIGBEE
#endif

#ifdef USE_IR_REMOTE
#undef USE_IR_REMOTE
#endif

#ifdef USE_IR_RECEIVE
#undef USE_IR_RECEIVE
#endif

#ifdef USE_RC_SWITCH
#undef USE_RC_SWITCH
#endif

#ifdef USE_SR04
#undef USE_SR04
#endif

#ifdef USE_RF_SENSOR
#undef USE_RF_SENSOR
#endif

#ifdef USE_ETHERNET
#undef USE_ETHERNET
#endif

#ifdef USE_PN532_HSU
#undef USE_PN532_HSU
#endif

#ifdef USE_RDM6300
#undef USE_RDM6300
#endif

#ifdef USE_IBEACON
#undef USE_IBEACON
#endif

#ifdef USE_BLE_ESP32
#undef USE_BLE_ESP32
#endif

#ifdef USE_MI_ESP32
#undef USE_MI_ESP32
#endif

#ifdef USE_MATTER_DEVICE
#undef USE_MATTER_DEVICE
#endif

#ifdef USE_SHUTTER
#undef USE_SHUTTER
#endif

#ifdef USE_SONOFF_RF
#undef USE_SONOFF_RF
#endif

#ifdef USE_RF_FLASH
#undef USE_RF_FLASH
#endif

#ifdef USE_SONOFF_SC
#undef USE_SONOFF_SC
#endif

#ifdef USE_TUYA_MCU
#undef USE_TUYA_MCU
#endif

#ifdef USE_ARMTRONIX_DIMMERS
#undef USE_ARMTRONIX_DIMMERS
#endif

#ifdef USE_PS_16_DZ
#undef USE_PS_16_DZ
#endif

#ifdef USE_SONOFF_IFAN
#undef USE_SONOFF_IFAN
#endif

#ifdef USE_BUZZER
#undef USE_BUZZER
#endif

#ifdef USE_ARILUX_RF
#undef USE_ARILUX_RF
#endif

#ifdef USE_DEEPSLEEP
#undef USE_DEEPSLEEP
#endif

#ifdef USE_EXS_DIMMER
#undef USE_EXS_DIMMER
#endif

#ifdef USE_HOTPLUG
#undef USE_HOTPLUG
#endif

#ifdef USE_DEVICE_GROUPS
#undef USE_DEVICE_GROUPS
#endif

#ifdef USE_PWM_DIMMER
#undef USE_PWM_DIMMER
#endif

#ifdef USE_PWM_DIMMER_REMOTE
#undef USE_PWM_DIMMER_REMOTE
#endif

#ifdef USE_KEELOQ
#undef USE_KEELOQ
#endif

#ifdef USE_SONOFF_D1
#undef USE_SONOFF_D1
#endif

#ifdef USE_SHELLY_DIMMER
#undef USE_SHELLY_DIMMER
#endif

#ifdef USE_FTP
#undef USE_FTP
#endif

#ifdef GUI_TRASH_FILE
#undef GUI_TRASH_FILE
#endif

#ifdef GUI_EDIT_FILE
#undef GUI_EDIT_FILE
#endif

#ifdef USE_GPIO_VIEWER
#undef USE_GPIO_VIEWER
#endif
#endif

// Match the stock Tasmota topic style so telemetry appears under
// tele/tasmota_xxxxxx/SENSOR by default.
#undef MQTT_TOPIC
#define MQTT_TOPIC        "tasmota_%06X"

// Use the same readable unique name for AP mode and DHCP hostname.
#ifdef WIFI_DEFAULT_HOSTNAME
#undef WIFI_DEFAULT_HOSTNAME
#endif
#define WIFI_DEFAULT_HOSTNAME "Pnks-%12X"

// Publish telemetry frequently enough to verify ENERGY payloads while testing.
#undef TELE_PERIOD
#define TELE_PERIOD       10

// Make the device easier to recognize in the web UI.
#undef FRIENDLY_NAME
#define FRIENDLY_NAME     "esp8685"

#ifdef DEVICE_NAME
#undef DEVICE_NAME
#endif
#define DEVICE_NAME       "esp8685"

#ifdef OTA_URL
#undef OTA_URL
#endif
#define OTA_URL "http://gym907-0001.iptime.org:3315/firmwareDownload/migration-main?projectId=10&chipType=esp8685"

#define USE_WIFI_CONFIG_ONLY_WEBUI

#ifdef ESP32
// Keep the web credentials, but stop forcing a compile-time template on ESP8685.
// This allows GPIO changes made in the Web UI to remain in flash settings instead
// of being reintroduced whenever defaults are regenerated.
#ifdef USER_TEMPLATE
#undef USER_TEMPLATE
#endif

#ifdef MODULE
#undef MODULE
#endif
#endif

#ifdef APP_LEDSTATE
#undef APP_LEDSTATE
#endif
#define APP_LEDSTATE      LED_POWER

#ifdef APP_ENABLE_LEDLINK
#undef APP_ENABLE_LEDLINK
#endif
#define APP_ENABLE_LEDLINK true

#ifndef USE_SMARTPLUG_CUSTOM
#define USE_SMARTPLUG_CUSTOM
#endif

#undef USER_BACKLOG
// [Rollback Fix Method 2]
// OTA 롤백을 방지하기 위한 2차 조치입니다. 첫 부팅 시, 타스모타가 자동으로
// "SetOption1 1"(설정 즉시 저장)과 "SaveData 1"(현재 상태 확정) 명령을 실행합니다.
// SaveData 명령은 내부적으로 애플리케이션을 유효한 것으로 표시하여 부트로더의 롤백을 막습니다.
//
// 가장 확실한 방법은 platformio.ini에 롤백 비활성화 빌드 플래그를 추가하는 것입니다.
// build_flags = -D CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=n
#define USER_BACKLOG "SetOption1 1; SaveData 1"

#endif  // _USER_CONFIG_OVERRIDE_H_
