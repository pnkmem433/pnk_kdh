#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

#warning **** user_config_override.h: Using Smart Plug Settings ****

#undef STA_SSID1
#define STA_SSID1         "CC-Retail"

#undef STA_PASS1
#define STA_PASS1         "pnks1111"

#undef MQTT_HOST
#define MQTT_HOST         "gym907-0001.iptime.org"

#undef MQTT_PORT
#define MQTT_PORT         1883

#undef MQTT_USER
#define MQTT_USER         "pnks"

#undef MQTT_PASS
#define MQTT_PASS         "pnks1111"

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
