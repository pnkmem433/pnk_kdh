#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

#warning **** user_config_override.h: Using Smart Plug Settings ****

// Force SECTION1 defaults to be reloaded on first boot after reflashing.
#undef CFG_HOLDER
#define CFG_HOLDER       4618

// Keep logs readable but avoid excessive UART traffic on the deployed smart plug.
#undef SERIAL_LOG_LEVEL
#define SERIAL_LOG_LEVEL LOG_LEVEL_INFO

#undef WEB_LOG_LEVEL
#define WEB_LOG_LEVEL    LOG_LEVEL_INFO

// Default network values for the custom smart-plug test image.
#undef STA_SSID1
#define STA_SSID1         "CC-Retail"

#undef STA_PASS1
#define STA_PASS1         "pnks1111"

#undef MQTT_HOST
#define MQTT_HOST         "192.168.0.15"

#undef MQTT_PORT
#define MQTT_PORT         1883

#undef MQTT_USER
#define MQTT_USER         "plugtest"

#undef MQTT_PASS
#define MQTT_PASS         "fcfc50kc35"

// Match the stock Tasmota topic style so telemetry appears under
// tele/tasmota_xxxxxx/SENSOR by default.
#undef MQTT_TOPIC
#define MQTT_TOPIC        "tasmota_%06X"

// Publish telemetry frequently enough to verify ENERGY payloads while testing.
#undef TELE_PERIOD
#define TELE_PERIOD       10

// Make the device easier to recognize in the web UI.
#undef FRIENDLY_NAME
#define FRIENDLY_NAME     "Tasmota"

#ifdef APP_LEDSTATE
#undef APP_LEDSTATE
#endif
#define APP_LEDSTATE      LED_POWER

#ifdef APP_ENABLE_LEDLINK
#undef APP_ENABLE_LEDLINK
#endif
#define APP_ENABLE_LEDLINK false

#ifndef USE_SMARTPLUG_CUSTOM
#define USE_SMARTPLUG_CUSTOM
#endif

#endif  // _USER_CONFIG_OVERRIDE_H_
