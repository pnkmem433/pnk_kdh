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
  // Extra fields not explicitly handled
  extraFields: Record<string, unknown>;
}

type ConnectionStatus = "connected" | "reconnecting" | "disconnected";

const BROKER_URL = "wss://newserver.pnklab.local/mqtt";
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
    ssid: null, ipAddress: null, lwt: null, extraFields: {},
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
      username: "plugtest", password: "fcfc50kc35",
    });
    clientRef.current = client;

    client.on("connect", () => {
      setConnectionStatus("connected");
      client.subscribe(
        ["smart_plug/+/status", "smart_plug/+/metrics", "tele/+/STATE", "tele/+/SENSOR", "tele/+/LWT"],
        (err) => { if (err) console.error("[MQTT] Subscribe error:", err); }
      );
    });

    client.on("reconnect", () => setConnectionStatus("reconnecting"));

    client.on("message", (topic, payload) => {
      const parts = topic.split("/");
      let obj: Record<string, unknown>;
      try { obj = JSON.parse(payload.toString()); } catch { return; }

      // smart_plug/{uuid}/status
      if (parts[0] === "smart_plug" && parts[2] === "status") {
        const uuid = parts[1];
        const extra = extractExtra(obj, KNOWN_STATUS_FIELDS);
        setPlugs((prev) => {
          const existing = prev[uuid] || makeDefaultPlug(uuid, getSavedName(uuid));
          return {
            ...prev,
            [uuid]: {
              ...existing,
              state: parseState(obj.state) ?? existing.state,
              webserver: numOrNull(obj.webserver) ?? existing.webserver,
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
        setPlugs((prev) => {
          const existing = prev[uuid] || makeDefaultPlug(uuid, getSavedName(uuid));
          return {
            ...prev,
            [uuid]: {
              ...existing,
              state: parseState(obj.state) ?? existing.state,
              webserver: numOrNull(obj.webserver) ?? existing.webserver,
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
        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
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

  return {
    plugs: Object.values(plugs),
    updateName,
    sendCommand,
    connectionStatus,
  };
}
