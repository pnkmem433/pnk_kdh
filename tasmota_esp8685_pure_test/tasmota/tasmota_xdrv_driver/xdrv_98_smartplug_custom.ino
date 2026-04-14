#ifdef USE_SMARTPLUG_CUSTOM
/*********************************************************************************************\
 * xdrv_98_smartplug_custom.ino - Smart plug command/status bridge for Tasmota
\*********************************************************************************************/

#define XDRV_98 98

namespace Spc98 {

const uint32_t kSmartplugStatusPeriodSeconds = 1;
uint32_t smartplug_seconds_until_publish = kSmartplugStatusPeriodSeconds;
char smartplug_uid[33] = { 0 };
char smartplug_topic_id[TOPSZ] = { 0 };
const char kSmartplugName[] = "Tasmota";
const char kSmartplugTemplateJson[] = "{\"NAME\":\"Tasmota\",\"ARCH\":\"esp32c3\",\"GPIO\":[0,0,0,0,224,0,320,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0],\"FLAG\":0,\"BASE\":1}";

bool SpCustomHandleCommand(const char* cmd);
bool SpCustomIsOn();

const char* SpCustomStateText() {
  return SpCustomIsOn() ? "on" : "off";
}

bool SpCustomTemplateMatches() {
  if (USER_MODULE != Settings->module) {
    return false;
  }
  if (strcmp(SettingsText(SET_TEMPLATE_NAME), kSmartplugName)) {
    return false;
  }
  if (Settings->user_template.gp.io[4] != GPIO_REL1) {
    return false;
  }
  if (Settings->user_template.gp.io[6] != GPIO_LED1_INV) {
    return false;
  }
  if (Settings->user_template.gp.io[20] != GPIO_KEY1) {
    return false;
  }
  return true;
}

void SpCustomEnsureDefaults() {
  bool changed = false;

  if (strcmp(SettingsText(SET_FRIENDLYNAME1), kSmartplugName)) {
    SettingsUpdateText(SET_FRIENDLYNAME1, kSmartplugName);
    changed = true;
  }
  if (strcmp(SettingsText(SET_DEVICENAME), kSmartplugName)) {
    SettingsUpdateText(SET_DEVICENAME, kSmartplugName);
    changed = true;
  }
  if (!SpCustomTemplateMatches()) {
    char template_json[sizeof(kSmartplugTemplateJson)];
    strlcpy(template_json, kSmartplugTemplateJson, sizeof(template_json));
    JsonTemplate(template_json);
    Settings->last_module = Settings->module;
    Settings->module = USER_MODULE;
    SetModuleType();
    changed = true;
  }

  if (changed) {
    AddLog(LOG_LEVEL_INFO, PSTR("SPC: Applied smart plug defaults and requesting restart"));
    SettingsSave(0);
    TasmotaGlobal.restart_flag = 2;
  }
}

void SpCustomLoadIds() {
  const String unique_id = NetworkUniqueId();
  strlcpy(smartplug_uid, unique_id.c_str(), sizeof(smartplug_uid));
  strlcpy(smartplug_topic_id, TasmotaGlobal.mqtt_topic, sizeof(smartplug_topic_id));
  AddLog(LOG_LEVEL_INFO, PSTR("SPC: UID %s, Topic %s"), smartplug_uid, smartplug_topic_id);
}

void SpCustomMakeTopic(char* buffer, size_t size, const char* base_id, const char* suffix) {
  snprintf_P(buffer, size, PSTR("smart_plug/%s/%s"), base_id, suffix);
}

void SpCustomMakeUidTopic(char* buffer, size_t size, const char* suffix) {
  SpCustomMakeTopic(buffer, size, smartplug_uid, suffix);
}

void SpCustomMakeTopicIdTopic(char* buffer, size_t size, const char* suffix) {
  SpCustomMakeTopic(buffer, size, smartplug_topic_id, suffix);
}

bool SpCustomIsOn() {
  return bitRead(TasmotaGlobal.power, 0);
}

void SpCustomMakeStatusPayload(char* buffer, size_t size) {
  snprintf_P(buffer, size, PSTR("{\"state\":\"%s\"}"), SpCustomStateText());
}

void SpCustomMakeMetricsPayload(char* buffer, size_t size) {
  if (Energy && TasmotaGlobal.energy_driver && Energy->phase_count) {
    const float voltage = Energy->voltage_available ? Energy->voltage[0] : 0.0f;
    const float current = Energy->current_available ? Energy->current[0] : 0.0f;
    const float power = Energy->active_power[0];
    const float total = Energy->total[0];
    const float daily = Energy->daily_sum;

    snprintf_P(
      buffer, size,
      PSTR("{\"state\":\"%s\",\"power\":%1_f,\"voltage\":%1_f,\"current\":%3_f,\"daily\":%3_f,\"total\":%3_f}"),
      SpCustomStateText(), &power, &voltage, &current, &daily, &total);
    return;
  }

  snprintf_P(buffer, size, PSTR("{\"state\":\"%s\",\"energy_available\":false}"), SpCustomStateText());
}

void SpCustomPublishPayloadToUid(const char* suffix, const char* payload) {
  char topic[TOPSZ];
  SpCustomMakeUidTopic(topic, sizeof(topic), suffix);
  MqttPublishPayload(topic, payload);
}

void SpCustomPublishStatus() {
  char payload[32];
  SpCustomMakeStatusPayload(payload, sizeof(payload));
  SpCustomPublishPayloadToUid("status", payload);
}

void SpCustomPublishMetrics() {
  char payload[160];
  SpCustomMakeMetricsPayload(payload, sizeof(payload));
  SpCustomPublishPayloadToUid("metrics", payload);
}

bool SpCustomTopicMatches(const char* suffix) {
  char expected_topic[TOPSZ];
  SpCustomMakeUidTopic(expected_topic, sizeof(expected_topic), suffix);
  if (!strcmp(XdrvMailbox.topic, expected_topic)) {
    return true;
  }

  if (strcmp(smartplug_uid, smartplug_topic_id)) {
    SpCustomMakeTopicIdTopic(expected_topic, sizeof(expected_topic), suffix);
    if (!strcmp(XdrvMailbox.topic, expected_topic)) {
      return true;
    }
  }

  return false;
}

bool SpCustomHandlePlainCommand(const char* payload) {
  if (!payload || !payload[0]) {
    return false;
  }

  // Support bare words and the old firmware style "cmd : ON".
  const char* command_text = payload;
  const char* separator = strchr(payload, ':');
  if (separator) {
    command_text = separator + 1;
    while (*command_text == ' ') {
      command_text++;
    }
  }

  return SpCustomHandleCommand(command_text);
}

bool SpCustomHandleCommand(const char* cmd) {
  if (!cmd || !cmd[0]) {
    return false;
  }

  if (!strcasecmp(cmd, "on")) {
    ExecuteCommandPower(1, POWER_ON, SRC_MQTT);
    SpCustomPublishStatus();
    SpCustomPublishMetrics();
    return true;
  }

  if (!strcasecmp(cmd, "off")) {
    ExecuteCommandPower(1, POWER_OFF, SRC_MQTT);
    SpCustomPublishStatus();
    SpCustomPublishMetrics();
    return true;
  }

  if (!strcasecmp(cmd, "status")) {
    SpCustomPublishStatus();
    SpCustomPublishMetrics();
    return true;
  }

  if (!strcasecmp(cmd, "toggle")) {
    ExecuteCommandPower(1, POWER_TOGGLE, SRC_MQTT);
    SpCustomPublishStatus();
    SpCustomPublishMetrics();
    MqttPublishSensor();
    return true;
  }

  return false;
}

void SpCustomSubscribe() {
  char topic[TOPSZ];
  SpCustomMakeUidTopic(topic, sizeof(topic), "command");
  MqttSubscribe(topic);
  if (strcmp(smartplug_uid, smartplug_topic_id)) {
    SpCustomMakeTopicIdTopic(topic, sizeof(topic), "command");
    MqttSubscribe(topic);
  }
}

bool SpCustomHandleMqttData() {
  if (!SpCustomTopicMatches("command")) {
    return false;
  }

  JsonParser parser((char*)XdrvMailbox.data);
  JsonParserObject root = parser.getRootObject();
  if (!root) {
    // Also accept plain payloads such as "on", "off", "toggle", "status"
    // and loose legacy payloads such as "cmd : ON".
    SpCustomHandlePlainCommand(XdrvMailbox.data);
    return true;
  }

  const char* cmd = root.getStr(PSTR("cmd"), "");
  SpCustomHandleCommand(cmd);
  return true;
}

void SpCustomInit() {
  smartplug_seconds_until_publish = kSmartplugStatusPeriodSeconds;
  SpCustomEnsureDefaults();
  SpCustomLoadIds();
}

void SpCustomEverySecond() {
  if (smartplug_seconds_until_publish > 0) {
    smartplug_seconds_until_publish--;
  }

  if (0 == smartplug_seconds_until_publish) {
    SpCustomPublishStatus();
    SpCustomPublishMetrics();
    MqttPublishSensor();
    smartplug_seconds_until_publish = kSmartplugStatusPeriodSeconds;
  }
}

}  // namespace Spc98

bool Xdrv98(uint32_t function) {
  switch (function) {
    case FUNC_INIT:
      Spc98::SpCustomInit();
      break;
    case FUNC_MQTT_SUBSCRIBE:
      Spc98::SpCustomSubscribe();
      break;
    case FUNC_MQTT_DATA:
      return Spc98::SpCustomHandleMqttData();
    case FUNC_SET_DEVICE_POWER:
      Spc98::SpCustomPublishStatus();
      Spc98::SpCustomPublishMetrics();
      MqttPublishSensor();
      break;
    case FUNC_EVERY_SECOND:
      Spc98::SpCustomEverySecond();
      break;
  }
  return false;
}

#endif  // USE_SMARTPLUG_CUSTOM
