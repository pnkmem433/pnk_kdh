#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

#warning "Building Tasmota SmartPlug lite-base non-minimal ESP02S profile"

#ifdef ESP8266
#ifdef CFG_HOLDER
#undef CFG_HOLDER
#endif
// Force a fresh Tasmota settings load so OTA upgrades from older/custom images
// cannot keep a stale module/template that maps the relay and LED incorrectly.
#define CFG_HOLDER 4630

#ifdef FALLBACK_MODULE
#undef FALLBACK_MODULE
#endif
#define FALLBACK_MODULE USER_MODULE

#ifdef BOOT_LOOP_OFFSET
#undef BOOT_LOOP_OFFSET
#endif
#define BOOT_LOOP_OFFSET 0

#ifndef USER_TEMPLATE
// ESP02S custom smart plug board:
// GPIO3  = Button
// GPIO13 = Blue LED (active low)
// GPIO14 = Relay
#define USER_TEMPLATE "{\"NAME\":\"esp02s\",\"GPIO\":[0,0,0,32,0,0,0,0,0,320,224,0,0,0],\"FLAG\":0,\"BASE\":18}"
#endif

#ifndef MODULE
#define MODULE USER_MODULE
#endif
#endif

#ifdef FRIENDLY_NAME
#undef FRIENDLY_NAME
#endif
#define FRIENDLY_NAME      "esp02s"

#undef STA_SSID1
#define STA_SSID1         "CC-Retail"

#undef STA_PASS1
#define STA_PASS1         "pnks1111"

#undef STA_SSID2
#define STA_SSID2         ""

#undef STA_PASS2
#define STA_PASS2         ""

#undef MQTT_HOST
#define MQTT_HOST         "api.pnkslab.com"

#undef MQTT_PORT
#define MQTT_PORT         1883

#undef MQTT_USER
#define MQTT_USER         "pnks"

#undef MQTT_PASS
#define MQTT_PASS         "pnks1111"

#ifdef WEB_USERNAME
#undef WEB_USERNAME
#endif
#define WEB_USERNAME      "pnkmem433"

#ifdef WEB_PASSWORD
#undef WEB_PASSWORD
#endif
#define WEB_PASSWORD      "pnks1111"

#ifdef APP_TIMEZONE
#undef APP_TIMEZONE
#endif
#define APP_TIMEZONE      9

#ifdef WIFI_DEFAULT_HOSTNAME
#undef WIFI_DEFAULT_HOSTNAME
#endif
#define WIFI_DEFAULT_HOSTNAME "Pnks-%12X"

#ifdef OTA_URL
#undef OTA_URL
#endif
#define OTA_URL "http://gym907-0001.iptime.org/ota/tasmota/esp02s/lite/esp02s_tasmota_lite.bin.gz"

#ifdef TELE_PERIOD
#undef TELE_PERIOD
#endif
#define TELE_PERIOD       10

#ifdef APP_LEDSTATE
#undef APP_LEDSTATE
#endif
#define APP_LEDSTATE      LED_POWER

#ifdef APP_ENABLE_LEDLINK
#undef APP_ENABLE_LEDLINK
#endif
#define APP_ENABLE_LEDLINK true

#ifdef WEB_SERVER
#undef WEB_SERVER
#endif
#define WEB_SERVER        2

#ifdef WEBSERVER_ADVERTISE
#undef WEBSERVER_ADVERTISE
#endif

#ifdef USE_WEB_STATUS_LINE
#undef USE_WEB_STATUS_LINE
#endif

#define USE_WIFI_CONFIG_ONLY_WEBUI

#ifndef USE_SMARTPLUG_CUSTOM
#define USE_SMARTPLUG_CUSTOM
#endif

// Discovery, integrations and emulation
#undef USE_ARDUINO_OTA
#undef USE_AUTOCONF
#undef USE_DISCOVERY
#undef USE_TASMOTA_DISCOVERY
#undef USE_HOME_ASSISTANT
#undef USE_DOMOTICZ
#undef USE_KNX
#undef USE_DALI
#undef USE_INFLUXDB
#undef USE_PROMETHEUS
#undef USE_TELEGRAM
#undef USE_UPNP
#undef USE_HUE
#undef USE_WEMO
#undef USE_EMULATION
#undef USE_EMULATION_HUE
#undef USE_EMULATION_WEMO
#undef USE_WEBSEND_RESPONSE

// Optional network helpers
#undef USE_MQTT_TLS
#undef USE_WEBCLIENT_HTTPS
#undef USE_MDNS
#undef USE_PING
#undef USE_SYSLOG
#undef USE_ENHANCED_GUI_WIFI_SCAN

// Automation / scripting
#undef USE_TIMERS
#undef USE_SUNRISE
#undef USE_RULES
#undef USE_EXPRESSION
#undef USE_SCRIPT
#undef USE_BERRY

// Filesystem / extra UI
#undef USE_UFILESYS
#undef USE_FTP
#undef GUI_TRASH_FILE
#undef GUI_EDIT_FILE
#undef USE_GPIO_VIEWER

// Buses, display, lighting, RF, IR, Tuya
#undef USE_I2C
#undef USE_SPI
#undef USE_DISPLAY
#undef USE_LIGHT
#undef USE_WS2812
#undef USE_IR_REMOTE
#undef USE_IR_RECEIVE
#undef USE_RC_SWITCH
#undef USE_SR04
#undef USE_TUYA_MCU
#undef USE_SERIAL_BRIDGE
#undef USE_MODBUS_BRIDGE

// Sensor families and energy measurement
#undef USE_ENERGY_SENSOR
#undef USE_ADC_VCC
#undef USE_COUNTER
#undef USE_DHT
#undef USE_DS18X20
#undef USE_DS18X20_LEGACY
#undef USE_BMP
#undef USE_BME680
#undef USE_BH1750
#undef USE_SHT
#undef USE_HTU
#undef USE_HX711

#endif  // _USER_CONFIG_OVERRIDE_H_
