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
bool smartplug_legacy_topics_cleared = false;

bool SpCustomHandleCommand(const char* cmd);
bool SpCustomIsOn();
uint32_t SpCustomWebServerMode();
void SpCustomClearCommandRetain();

const char* SpCustomStateText() {
  return SpCustomIsOn() ? "on" : "off";
}

uint32_t SpCustomWebServerMode() {
  return Settings->webserver;
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

void SpCustomClearLegacyTopicRetain(const char* suffix) {
  if (!strcmp(smartplug_uid, smartplug_topic_id)) {
    return;
  }

  char topic[TOPSZ];
  SpCustomMakeTopicIdTopic(topic, sizeof(topic), suffix);
  MqttPublishPayload(topic, "", 0, true);
}

void SpCustomClearUidTopicRetain(const char* suffix) {
  char topic[TOPSZ];
  SpCustomMakeUidTopic(topic, sizeof(topic), suffix);
  MqttPublishPayload(topic, "", 0, true);
}

void SpCustomClearCommandRetain() {
  // Commands should not persist across reboots. Clear any retained command on both
  // the UID topic and the legacy topic alias so old webserver commands do not replay.
  SpCustomClearUidTopicRetain("command");
  SpCustomClearLegacyTopicRetain("command");
}

bool SpCustomIsOn() {
  return bitRead(TasmotaGlobal.power, 0);
}

void SpCustomMakeStatusPayload(char* buffer, size_t size) {
  snprintf_P(
    buffer, size,
    PSTR("{\"state\":\"%s\",\"webserver\":%u}"),
    SpCustomStateText(), SpCustomWebServerMode());
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

void SpCustomClearLegacyAliasTopicsOnce() {
  if (smartplug_legacy_topics_cleared) {
    return;
  }

  if (strcmp(smartplug_uid, smartplug_topic_id)) {
    SpCustomClearLegacyTopicRetain("status");
  }

  smartplug_legacy_topics_cleared = true;
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

  if (!strcasecmp(cmd, "webserver 0")) {
    ExecuteCommand((char*)"Backlog WebServer 0; Restart 1", SRC_MQTT);
    return true;
  }

  if (!strcasecmp(cmd, "webserver 1")) {
    ExecuteCommand((char*)"Backlog WebPassword 0; WebServer 1; Restart 1", SRC_MQTT);
    return true;
  }

  if (!strcasecmp(cmd, "webserver 2")) {
    ExecuteCommand((char*)"Backlog WebServer 2; WebPassword pnks1111; Restart 1", SRC_MQTT);
    return true;
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

  if (!strcasecmp(cmd, "toggle")) {
    ExecuteCommandPower(1, POWER_TOGGLE, SRC_MQTT);
    SpCustomPublishStatus();
    MqttPublishSensor();
    return true;
  }

  // Pass through any other command to the native Tasmota command parser so
  // MQTT clients can use WifiConfig, OtaUrl, Upgrade, Restart and similar
  // commands through the custom smart_plug/<uid>/command topic.
  ExecuteCommand((char*)cmd, SRC_MQTT);
  return true;
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
  smartplug_legacy_topics_cleared = false;
  SpCustomLoadIds();
}

void SpCustomEverySecond() {
  if (smartplug_seconds_until_publish > 0) {
    smartplug_seconds_until_publish--;
  }

  if (0 == smartplug_seconds_until_publish) {
    SpCustomClearLegacyAliasTopicsOnce();
    SpCustomPublishStatus();
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
    case FUNC_MQTT_INIT:
      Spc98::SpCustomClearCommandRetain();
      break;
    case FUNC_MQTT_SUBSCRIBE:
      Spc98::SpCustomSubscribe();
      break;
    case FUNC_MQTT_DATA:
      return Spc98::SpCustomHandleMqttData();
    case FUNC_SET_DEVICE_POWER:
      Spc98::SpCustomPublishStatus();
      MqttPublishSensor();
      break;
    case FUNC_EVERY_SECOND:
      Spc98::SpCustomEverySecond();
      break;
  }
  return false;
}

#endif  // USE_SMARTPLUG_CUSTOM
