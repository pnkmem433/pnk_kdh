#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define FW_VER_MAJOR 0
#define FW_VER_MINOR 7
#define FW_VER_PATCH 4

#define FW_VERSION_CODE ((FW_VER_MAJOR * 100) + (FW_VER_MINOR * 10) + FW_VER_PATCH)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define FW_VERSION_STRING STR(FW_VER_MAJOR) "." STR(FW_VER_MINOR) "." STR(FW_VER_PATCH)

#define WIFI_SSID "plugtest"
#define WIFI_PASSWORD "fcfc50kc35"
#define MQTT_HOST "api.pnkslab.com"
#define MQTT_PORT 1884
#define MQTT_USER "pnks"
#define MQTT_PASSWORD "pnks1111"
#define OTA_URL "http://192.168.0.84:3004"
#define OTA_PROJECT_ID 10
#define CURRENT_FIRMWARE_FAMILY "custom"
#endif
