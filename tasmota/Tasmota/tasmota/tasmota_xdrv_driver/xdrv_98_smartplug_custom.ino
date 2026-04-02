#ifdef USE_SMARTPLUG_CUSTOM
/*********************************************************************************************\
 * xdrv_98_smartplug_custom.ino - Smart plug command/status bridge for Tasmota
\*********************************************************************************************/

#define XDRV_98 98

namespace Spc98 {

const uint32_t kSmartplugStatusPeriodSeconds = 5;
const uint32_t kBootBlinkTicks = 40;      // 40 x 50ms = 2 seconds
const uint32_t kOtaBlinkTicks = 2;        // 2 x 50ms = 100ms
uint32_t smartplug_seconds_until_publish = kSmartplugStatusPeriodSeconds;
uint32_t smartplug_boot_blink_ticks = 0;
uint32_t smartplug_ota_blink_ticks = 0;
bool smartplug_led_state = false;

void SpCustomMakeTopic(char* buffer, size_t size, const char* suffix) {
  snprintf_P(buffer, size, PSTR("smart_plug/%s/%s"), TasmotaGlobal.mqtt_topic, suffix);
}

bool SpCustomIsOn() {
  return bitRead(TasmotaGlobal.power, 0);
}

void SpCustomMakeStatusPayload(char* buffer, size_t size) {
  snprintf_P(buffer, size, PSTR("{\"state\":\"%s\"}"), GetStateText(SpCustomIsOn()));
}

void SpCustomPublishStatus() {
  char topic[TOPSZ];
  char payload[32];
  SpCustomMakeTopic(topic, sizeof(topic), "status");
  SpCustomMakeStatusPayload(payload, sizeof(payload));
  MqttPublishPayload(topic, payload);
}

bool SpCustomHandleCommand(const char* cmd) {
  if (!cmd || !cmd[0]) {
    return false;
  }

  if (!strcasecmp(cmd, "on")) {
    ExecuteCommandPower(1, POWER_ON, SRC_MQTT);
    SpCustomPublishStatus();
    return true;
  }

  if (!strcasecmp(cmd, "off")) {
    ExecuteCommandPower(1, POWER_OFF, SRC_MQTT);
    SpCustomPublishStatus();
    return true;
  }

  if (!strcasecmp(cmd, "status")) {
    SpCustomPublishStatus();
    return true;
  }

  return false;
}

void SpCustomSubscribe() {
  char topic[TOPSZ];
  SpCustomMakeTopic(topic, sizeof(topic), "command");
  MqttSubscribe(topic);
}

bool SpCustomHandleMqttData() {
  char expected_topic[TOPSZ];
  SpCustomMakeTopic(expected_topic, sizeof(expected_topic), "command");

  if (strcmp(XdrvMailbox.topic, expected_topic)) {
    return false;
  }

  JsonParser parser((char*)XdrvMailbox.data);
  JsonParserObject root = parser.getRootObject();
  if (!root) {
    return true;
  }

  const char* cmd = root.getStr(PSTR("cmd"), "");
  SpCustomHandleCommand(cmd);
  return true;
}

void SpCustomInit() {
  smartplug_seconds_until_publish = kSmartplugStatusPeriodSeconds;
  smartplug_boot_blink_ticks = kBootBlinkTicks;
  smartplug_ota_blink_ticks = kOtaBlinkTicks;
  smartplug_led_state = false;
}

void SpCustomEverySecond() {
  if (smartplug_seconds_until_publish > 0) {
    smartplug_seconds_until_publish--;
  }

  if (0 == smartplug_seconds_until_publish) {
    SpCustomPublishStatus();
    smartplug_seconds_until_publish = kSmartplugStatusPeriodSeconds;
  }
}

void SpCustomEvery50mSecond() {
  if (!PinUsed(GPIO_LEDLNK)) {
    return;
  }

  if (smartplug_boot_blink_ticks > 0) {
    smartplug_boot_blink_ticks--;
    smartplug_led_state = !smartplug_led_state;
    SetLedLink(smartplug_led_state);
    return;
  }

  if (TasmotaGlobal.ota_state_flag) {
    if (smartplug_ota_blink_ticks > 0) {
      smartplug_ota_blink_ticks--;
    }
    if (0 == smartplug_ota_blink_ticks) {
      smartplug_ota_blink_ticks = kOtaBlinkTicks;
      smartplug_led_state = !smartplug_led_state;
      SetLedLink(smartplug_led_state);
    }
    return;
  }

  smartplug_ota_blink_ticks = kOtaBlinkTicks;
}

}  // namespace Spc98

bool Xdrv98(uint32_t function) {
  switch (function) {
    case FUNC_INIT:
      Spc98::SpCustomInit();
      break;
    case FUNC_EVERY_50_MSECOND:
      Spc98::SpCustomEvery50mSecond();
      break;
    case FUNC_MQTT_SUBSCRIBE:
      Spc98::SpCustomSubscribe();
      break;
    case FUNC_MQTT_DATA:
      return Spc98::SpCustomHandleMqttData();
    case FUNC_SET_DEVICE_POWER:
      Spc98::SpCustomPublishStatus();
      break;
    case FUNC_EVERY_SECOND:
      Spc98::SpCustomEverySecond();
      break;
  }
  return false;
}

#endif  // USE_SMARTPLUG_CUSTOM
