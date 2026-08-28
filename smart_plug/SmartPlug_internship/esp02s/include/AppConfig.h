#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define FW_VER_MAJOR 0
#define FW_VER_MINOR 1
#define FW_VER_PATCH 5

#define FW_VERSION_CODE ((FW_VER_MAJOR * 100) + (FW_VER_MINOR * 10) + FW_VER_PATCH)

/*
 * WiFi:
 * - Saved Wi-Fi credentials from WiFiManager are preferred after reboot.
 * - WIFI_SSID / WIFI_PASSWORD are only used as a fallback when no saved Wi-Fi exists
 *   or the saved network cannot be connected.
 */

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define FW_VERSION_STRING STR(FW_VER_MAJOR) "." STR(FW_VER_MINOR) "." STR(FW_VER_PATCH)

#define WIFI_SSID "lab-2"
#define WIFI_PASSWORD "pnks1111"
#define MQTT_HOST "api.pnkslab.com"
#define MQTT_PORT 1883
#define MQTT_USER "pnks"
#define MQTT_PASSWORD "pnks1111"
#define OTA_URL "http://gym907-0001.iptime.org:3315"
#define OTA_PROJECT_ID 10
#define CHIP_TYPE "esp02s"
#define CURRENT_FIRMWARE_FAMILY "custom"
#endif
