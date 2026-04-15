import { useState, useEffect, useCallback, useRef } from "react";
import mqtt from "mqtt";

export interface PlugData {
  uuid: string;
  state: "on" | "off";
  name: string;
  lastSeen: number | null;
  voltage: number | null;
  offline: boolean;
}

type ConnectionStatus = "connected" | "reconnecting" | "disconnected";

const BROKER_URL = "wss://newserver.pnklab.local/mqtt";
const OFFLINE_THRESHOLD = 35_000; // 35 seconds

export function useMqttPlugs() {
  const [plugs, setPlugs] = useState<Record<string, PlugData>>({});
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>("disconnected");
  const clientRef = useRef<mqtt.MqttClient | null>(null);

  const getSavedName = (uuid: string) => {
    try {
      const names = JSON.parse(localStorage.getItem("plug_names") || "{}");
      return names[uuid] || `Plug ${uuid.slice(-4)}`;
    } catch {
      return `Plug ${uuid.slice(-4)}`;
    }
  };

  const saveName = (uuid: string, name: string) => {
    try {
      const names = JSON.parse(localStorage.getItem("plug_names") || "{}");
      names[uuid] = name;
      localStorage.setItem("plug_names", JSON.stringify(names));
    } catch { /* noop */ }
  };

  // Find full UUID by short id (last 6 chars)
  const findUuidByShortId = (shortId: string, current: Record<string, PlugData>): string | null => {
    const upper = shortId.toUpperCase();
    return Object.keys(current).find((uuid) => uuid.toUpperCase().endsWith(upper)) || null;
  };

  useEffect(() => {
    console.log("[MQTT] Connecting to", BROKER_URL);
    const client = mqtt.connect(BROKER_URL, {
      protocolVersion: 5,
      reconnectPeriod: 3000,
      connectTimeout: 10000,
      username: "plugtest",
      password: "fcfc50kc35",
    });
    clientRef.current = client;

    client.on("connect", () => {
      console.log("[MQTT] Connected!");
      setConnectionStatus("connected");
      client.subscribe(
        ["smart_plug/+/status", "smart_plug/+/metrics", "tele/+/STATE", "tele/+/SENSOR"],
        (err) => {
          if (err) console.error("[MQTT] Subscribe error:", err);
          else console.log("[MQTT] Subscribed to all topics");
        }
      );
    });

    client.on("reconnect", () => {
      console.log("[MQTT] Reconnecting...");
      setConnectionStatus("reconnecting");
    });

    client.on("message", (topic, payload) => {
      const parts = topic.split("/");
      let payloadObj: Record<string, unknown>;
      try {
        payloadObj = JSON.parse(payload.toString());
      } catch {
        return;
      }

      // smart_plug/{uuid}/status
      if (parts[0] === "smart_plug" && parts[2] === "status") {
        const uuid = parts[1];
        const rawState = String(payloadObj.state || "").toLowerCase();
        const state: "on" | "off" = rawState === "on" ? "on" : "off";

        setPlugs((prev) => ({
          ...prev,
          [uuid]: {
            uuid,
            state,
            name: prev[uuid]?.name || getSavedName(uuid),
            lastSeen: Date.now(),
            voltage: prev[uuid]?.voltage ?? null,
            offline: false,
          },
        }));
        return;
      }

      // smart_plug/{uuid}/metrics
      if (parts[0] === "smart_plug" && parts[2] === "metrics") {
        const uuid = parts[1];
        setPlugs((prev) => {
          if (!prev[uuid]) return prev;
          const rawState = String(payloadObj.state || prev[uuid].state).toLowerCase();
          return {
            ...prev,
            [uuid]: {
              ...prev[uuid],
              state: rawState === "on" ? "on" : "off",
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
        const energy = payloadObj.ENERGY as Record<string, unknown> | undefined;
        if (!energy) return;
        const voltage = typeof energy.Voltage === "number" ? energy.Voltage : null;

        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
          return {
            ...prev,
            [fullUuid]: {
              ...prev[fullUuid],
              voltage: voltage ?? prev[fullUuid].voltage,
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
        setPlugs((prev) => {
          const fullUuid = findUuidByShortId(shortId, prev);
          if (!fullUuid) return prev;
          return {
            ...prev,
            [fullUuid]: {
              ...prev[fullUuid],
              lastSeen: Date.now(),
              offline: false,
            },
          };
        });
      }
    });

    client.on("error", (err) => {
      console.error("[MQTT] Error:", err);
    });

    client.on("close", () => {
      console.log("[MQTT] Disconnected");
      setConnectionStatus("disconnected");
    });

    // Offline checker
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

    return () => {
      clearInterval(offlineInterval);
      client.end();
    };
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
    if (!client || !client.connected) {
      console.warn("[MQTT] Not connected, cannot send command");
      return;
    }
    const topic = `smart_plug/${uuid}/command`;
    const payload = JSON.stringify({ cmd });
    client.publish(topic, payload, { qos: 1 }, (err) => {
      if (err) console.error("[MQTT] Publish error:", err);
      else console.log(`[MQTT] Published ${payload} to ${topic}`);
    });
  }, []);

  return {
    plugs: Object.values(plugs),
    updateName,
    sendCommand,
    connectionStatus,
  };
}
