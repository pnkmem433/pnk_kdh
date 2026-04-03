#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

#warning **** user_config_override.h: Using Smart Plug Settings ****

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

#ifdef APP_TIMEZONE
#undef APP_TIMEZONE
#endif
#define APP_TIMEZONE      9

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
#define APP_ENABLE_LEDLINK false

#ifndef USE_SMARTPLUG_CUSTOM
#define USE_SMARTPLUG_CUSTOM
#endif

#endif  // _USER_CONFIG_OVERRIDE_H_
