#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

// ?????????????????????? ?????? ??? ??? ???
#warning "**** Building Custom Tasmota SmartPlug Firmware (Aggressive Size Optimization) ****"

// --- 1. ??? ??? (Wi-Fi, MQTT) ---
#undef STA_SSID1
#define STA_SSID1 "CC-Retail"
#undef STA_PASS1
#define STA_PASS1 "pnks1111"

#undef MQTT_HOST
#define MQTT_HOST "api.pnkslab.com"
#undef MQTT_PORT
#define MQTT_PORT 1884
#undef MQTT_USER
#define MQTT_USER "pnks"
#undef MQTT_PASS
#define MQTT_PASS "pnks1111"

// --- 2. ???????? ???????? ???????? ?????? ---
// ???????????? ?????? ???
#undef USE_DOMOTICZ
#undef USE_HOME_ASSISTANT
#undef USE_DISCOVERY
#undef USE_TASMOTA_DISCOVERY
#undef USE_KNX
#undef USE_PROMETHEUS
#undef USE_WEMO
#undef USE_HUE
#undef USE_EMULATION_HUE
#undef USE_EMULATION_WEMO
#undef USE_UPNP
#undef USE_TIMERS
#undef USE_RULES
#undef USE_EXPRESSION
#undef USE_SCRIPT
#undef USE_PING
#undef USE_MDNS
#undef USE_SYSLOG

// ?????? ??? ?????????????
#undef USE_I2C
#undef USE_SPI
#undef USE_DISPLAY
#undef USE_IR_REMOTE
#undef USE_IR_RECEIVE
#undef USE_WS2812
#undef USE_SR04
#undef USE_RC_SWITCH
#undef USE_TUYA_MCU
#undef USE_ENERGY_SENSOR
#undef USE_SERIAL_BRIDGE
#undef USE_COUNTER

// ??? ?????? ??? ?????? ???
#undef USE_DHT
#undef USE_DS18X20
#undef USE_DS18X20_LEGACY
#undef USE_BMP
#undef USE_BME680
#undef USE_BH1750
#undef USE_SHT
#undef USE_HTU
#undef USE_ADC_VCC

#endif  // _USER_CONFIG_OVERRIDE_H_
