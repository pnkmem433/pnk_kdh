import { useState, useEffect, useCallback, useRef } from "react";
import mqtt from "mqtt";

export interface PlugData {
  uuid: string;
  name: string;
  state: "on" | "off" | null;
  webserver: number | null;
  lastSeen: number | null;
  offline: boolean;
  // Energy
  energyAvailable: boolean | null;
  power: number | null;
  voltage: number | null;
  current: number | null;
  daily: number | null;
  total: number | null;
  // Network (from tele STATE)
  ssid: string | null;
  ipAddress: string | null;
  // LWT (Last Will & Testament) — Tasmota Online/Offline
  lwt: "Online" | "Offline" | null;
  // Module type from tele INFO1 (e.g. "esp02s", "esp8685", "ESP32C3")
  module: string | null;
  // Extra fields not explicitly handled
  extraFields: Record<string, unknown>;
}

type ConnectionStatus = "connected" | "reconnecting" | "disconnected";

const BROKER_URL = "wss://api.pnkslab.com/mqtt";
const OFFLINE_THRESHOLD = 35_000;

// Known fields we extract explicitly — everything else goes to extraFields
const KNOWN_STATUS_FIELDS = new Set(["state", "webserver"]);
const KNOWN_METRICS_FIELDS = new Set([
  "state", "webserver", "energy_available",
  "power", "voltage", "current", "daily", "total",
]);

function parseState(val: unknown): "on" | "off" | null {
  if (typeof val !== "string") return null;
  const lower = val.toLowerCase();
  return lower === "on" ? "on" : lower === "off" ? "off" : null;
}

function numOrNull(val: unknown): number | null {
  return typeof val === "number" ? val : null;
}

function extractExtra(payload: Record<string, unknown>, known: Set<string>): Record<string, unknown> {
  const extra: Record<string, unknown> = {};
  for (const key of Object.keys(payload)) {
    if (!known.has(key)) extra[key] = payload[key];
  }
  return extra;
}

function makeDefaultPlug(uuid: string, name: string): PlugData {
  return {
    uuid, name, state: null, webserver: null, lastSeen: null, offline: false,
    energyAvailable: null, power: null, voltage: null, current: null, daily: null, total: null,
    ssid: null, ipAddress: null, lwt: null, module: null, extraFields: {},
  };
}

export function useMqttPlugs() {
  const [plugs, setPlugs] = useState<Record<string, PlugData>>({});
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>("disconnected");
  const clientRef = useRef<mqtt.MqttClient | null>(null);

  const getSavedName = (uuid: string) => {
    try {
      const names = JSON.parse(localStorage.getItem("plug_names") || "{}");
      return names[uuid] || `Plug ${uuid.slice(-4)}`;
    } catch { return `Plug ${uuid.slice(-4)}`; }
  };

  const saveName = (uuid: string, name: string) => {
    try {
      const names = JSON.parse(localStorage.getItem("plug_names") || "{}");
      names[uuid] = name;
      localStorage.setItem("plug_names", JSON.stringify(names));
    } catch { /* noop */ }
  };

  const findUuidByShortId = (shortId: string, current: Record<string, PlugData>): string | null => {
    const upper = shortId.toUpperCase();
    return Object.keys(current).find((uuid) => uuid.toUpperCase().endsWith(upper)) || null;
  };

  useEffect(() => {
    const client = mqtt.connect(BROKER_URL, {
      protocolVersion: 5, reconnectPeriod: 3000, connectTimeout: 10000,
      username: "pnks", password: "pnks1111",
    });
    clientRef.current = client;

    client.on("connect", () => {
      setConnectionStatus("connected");
      client.subscribe(
        ["smart_plug/+/status", "smart_plug/+/metrics", "smart_plug/+/wifi", "tele/+/STATE", "tele/+/SENSOR", "tele/+/LWT", "tele/+/INFO1", "stat/+/STATUS2"],
        (err) => { if (err) console.error("[MQTT] Subscribe error:", err); }
      );
    });

    client.on("reconnect", () => setConnectionStatus("reconnecting"));

    client.on("message", (topic, payload) => {
      const parts = topic.split("/");
      const payloadStr = payload.toString();

      // tele/tasmota_XXXXXX/LWT — plain text "Online" / "Offline"
      if (parts[0] === "tele" && parts[2] === "LWT") {
        const shortId = parts[1].replace("tasmota_", "");
        const lwtVal = payloadStr === "Online" ? "Online" : "Offline";
        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
          return {
            ...prev,
            [fullUuid]: { ...prev[fullUuid], lwt: lwtVal },
          };
        });
        return;
      }

      let obj: Record<string, unknown>;
      try { obj = JSON.parse(payloadStr); } catch { return; }

      // tele/tasmota_XXXXXX/INFO1 — extract Module (esp02s, ESP32C3, esp8685, ...)
      if (parts[0] === "tele" && parts[2] === "INFO1") {
        const shortId = parts[1].replace("tasmota_", "");
        const info1 = obj.Info1 as Record<string, unknown> | undefined;
        const moduleVal = typeof info1?.Module === "string" ? (info1.Module as string) : null;
        if (!moduleVal) return;
        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
          return {
            ...prev,
            [fullUuid]: { ...prev[fullUuid], module: moduleVal },
          };
        });
        return;
      }

      // stat/tasmota_XXXXXX/STATUS2 — response to "Status 2", contains Hardware
      // {"StatusFWR":{"Version":"15.3.0.3(lite)","Hardware":"ESP32-C3", ...}}
      if (parts[0] === "stat" && parts[2] === "STATUS2") {
        const shortId = parts[1].replace("tasmota_", "");
        const fwr = obj.StatusFWR as Record<string, unknown> | undefined;
        const hw = typeof fwr?.Hardware === "string" ? (fwr.Hardware as string) : null;
        if (!hw) return;
        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
          return {
            ...prev,
            [fullUuid]: { ...prev[fullUuid], module: hw },
          };
        });
        return;
      }

      // smart_plug/{uuid}/status
      if (parts[0] === "smart_plug" && parts[2] === "status") {
        const uuid = parts[1];
        const extra = extractExtra(obj, KNOWN_STATUS_FIELDS);
        const hasWebserver = Object.prototype.hasOwnProperty.call(obj, "webserver");
        setPlugs((prev) => {
          const existing = prev[uuid] || makeDefaultPlug(uuid, getSavedName(uuid));
          return {
            ...prev,
            [uuid]: {
              ...existing,
              state: parseState(obj.state) ?? existing.state,
              // If webserver key is absent → firmware is 자체제작, reset to null.
              // If present → use its numeric value (or null if not numeric).
              webserver: hasWebserver ? numOrNull(obj.webserver) : null,
              lastSeen: Date.now(),
              offline: false,
              extraFields: { ...existing.extraFields, ...extra },
            },
          };
        });
        return;
      }

      // smart_plug/{uuid}/metrics
      if (parts[0] === "smart_plug" && parts[2] === "metrics") {
        const uuid = parts[1];
        const extra = extractExtra(obj, KNOWN_METRICS_FIELDS);
        const hasWebserver = Object.prototype.hasOwnProperty.call(obj, "webserver");
        setPlugs((prev) => {
          const existing = prev[uuid] || makeDefaultPlug(uuid, getSavedName(uuid));
          return {
            ...prev,
            [uuid]: {
              ...existing,
              state: parseState(obj.state) ?? existing.state,
              webserver: hasWebserver ? numOrNull(obj.webserver) : null,
              energyAvailable: typeof obj.energy_available === "boolean" ? obj.energy_available : existing.energyAvailable,
              power: numOrNull(obj.power) ?? existing.power,
              voltage: numOrNull(obj.voltage) ?? existing.voltage,
              current: numOrNull(obj.current) ?? existing.current,
              daily: numOrNull(obj.daily) ?? existing.daily,
              total: numOrNull(obj.total) ?? existing.total,
              lastSeen: Date.now(),
              offline: false,
              extraFields: { ...existing.extraFields, ...extra },
            },
          };
        });
        return;
      }

      // smart_plug/{uuid}/wifi — 자체제작 펌웨어 WiFi 정보
      // 두 가지 형식 지원:
      // 1) {"Wifi":{"SSID":"...","SSId":"...","BSSId":"...",...}}
      // 2) {"configuredSsid":"...","currentSsid":"...","connected":true,"localIp":"192.168.1.168"}
      if (parts[0] === "smart_plug" && parts[2] === "wifi") {
        const uuid = parts[1];
        const wifi = obj.Wifi as Record<string, unknown> | undefined;
        const ssid =
          (typeof wifi?.SSID === "string" && wifi.SSID) ||
          (typeof wifi?.SSId === "string" && wifi.SSId) ||
          (typeof obj.currentSsid === "string" && obj.currentSsid) ||
          (typeof obj.configuredSsid === "string" && obj.configuredSsid) ||
          null;
        const ip =
          (typeof obj.localIp === "string" && obj.localIp) ||
          (typeof obj.ip === "string" && obj.ip) ||
          (typeof wifi?.IPAddress === "string" && (wifi.IPAddress as string)) ||
          null;
        setPlugs((prev) => {
          const existing = prev[uuid] || makeDefaultPlug(uuid, getSavedName(uuid));
          return {
            ...prev,
            [uuid]: {
              ...existing,
              ssid: ssid || existing.ssid,
              ipAddress: ip || existing.ipAddress,
              lastSeen: Date.now(),
              offline: false,
            },
          };
        });
        return;
      }

      // tele/tasmota_XXXXXX/SENSOR
      if (parts[0] === "tele" && parts[2] === "SENSOR") {
        const shortId = parts[1].replace("tasmota_", "");
        const energy = obj.ENERGY as Record<string, unknown> | undefined;
        if (!energy) return;
        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
          return {
            ...prev,
            [fullUuid]: {
              ...prev[fullUuid],
              voltage: numOrNull(energy.Voltage) ?? prev[fullUuid].voltage,
              power: numOrNull(energy.Power) ?? prev[fullUuid].power,
              current: numOrNull(energy.Current) ?? prev[fullUuid].current,
              daily: numOrNull(energy.Today) ?? prev[fullUuid].daily,
              total: numOrNull(energy.Total) ?? prev[fullUuid].total,
              lastSeen: Date.now(),
              offline: false,
            },
          };
        });
        return;
      }

      // tele/tasmota_XXXXXX/STATE
      if (parts[0] === "tele" && parts[2] === "STATE") {
        const shortId = parts[1].replace("tasmota_", "");
        const wifi = obj.Wifi as Record<string, unknown> | undefined;
        const ssid = typeof wifi?.SSId === "string" ? wifi.SSId : null;
        const ip = typeof obj.IPAddress === "string" ? (obj.IPAddress as string) : null;
        let needStatusReq = false;
        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
          if (!prev[fullUuid].module) needStatusReq = true;
          return {
            ...prev,
            [fullUuid]: {
              ...prev[fullUuid],
              ssid: ssid ?? prev[fullUuid].ssid,
              ipAddress: ip ?? prev[fullUuid].ipAddress,
              lastSeen: Date.now(),
              offline: false,
            },
          };
        });
        // Module이 아직 없으면 Status 2 요청 → stat/.../STATUS2 응답으로 Hardware 수신
        if (needStatusReq && client.connected) {
          client.publish(`cmnd/tasmota_${shortId}/Status`, "2");
        }
      }
    });

    client.on("error", (err) => console.error("[MQTT] Error:", err));
    client.on("close", () => setConnectionStatus("disconnected"));

    const offlineInterval = setInterval(() => {
      const now = Date.now();
      setPlugs((prev) => {
        let changed = false;
        const next = { ...prev };
        for (const uuid of Object.keys(next)) {
          const plug = next[uuid];
          const isOffline = plug.lastSeen !== null && now - plug.lastSeen > OFFLINE_THRESHOLD;
          if (isOffline !== plug.offline) {
            changed = true;
            next[uuid] = { ...plug, offline: isOffline };
          }
        }
        return changed ? next : prev;
      });
    }, 5000);

    return () => { clearInterval(offlineInterval); client.end(); };
  }, []);

  const updateName = useCallback((uuid: string, newName: string) => {
    saveName(uuid, newName);
    setPlugs((prev) => ({
      ...prev,
      [uuid]: { ...prev[uuid], name: newName },
    }));
  }, []);

  const sendCommand = useCallback((uuid: string, cmd: "on" | "off") => {
    const client = clientRef.current;
    if (client && client.connected) {
      client.publish(`smart_plug/${uuid}/command`, JSON.stringify({ cmd }));
    }
  }, []);

  const plugsRef = useRef<Record<string, PlugData>>({});
  useEffect(() => { plugsRef.current = plugs; }, [plugs]);

  const sendOta = useCallback((uuid: string, kind: "tasmota" | "custom"): boolean => {
    const client = clientRef.current;
    if (!client || !client.connected) return false;
    const shortId = uuid.slice(-6).toUpperCase();
    // Module 판별: tele INFO1에서 가져온 module 값 (esp02s, ESP32C3, esp8685 ...)
    const mod = (plugsRef.current[uuid]?.module || "").toLowerCase().replace(/[-_\s]/g, "");
    // ESP32-C3 / ESP8685 계열 → esp8685 폴더, 그 외(기본 esp02s/ESP8285) → esp02s 폴더
    const family = mod.includes("8685") || mod.includes("32c3")
      ? "esp8685"
      : "esp02s";
    const url = kind === "tasmota"
      ? `http://gym907-0001.iptime.org/ota/tasmota/${family}/lite/${family}_lite.bin`
      : `http://gym907-0001.iptime.org/ota/tasmota/${family}/custom/${family}_custom.bin`;
    const payload = `OtaUrl ${url}; Upgrade 1`;
    client.publish(`cmnd/tasmota_${shortId}/Backlog`, payload);
    return true;
  }, []);

  return {
    plugs: Object.values(plugs),
    updateName,
    sendCommand,
    sendOta,
    connectionStatus,
  };
}
