import { useState, useEffect, useCallback, useRef } from "react";
import mqtt from "mqtt";

export interface PlugData {
  uuid: string;
  state: "on" | "off";
  name: string;
}

const BROKER_URL = "wss://newserver.pnklab.local/mqtt";

export function useMqttPlugs() {
  const [plugs, setPlugs] = useState<Record<string, PlugData>>({});
  const [connected, setConnected] = useState(false);
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

  useEffect(() => {
    console.log("[MQTT] Connecting to", BROKER_URL);
    const client = mqtt.connect(BROKER_URL, {
      protocolVersion: 5,
      reconnectPeriod: 5000,
      connectTimeout: 10000,
    });
    clientRef.current = client;

    client.on("connect", () => {
      console.log("[MQTT] Connected!");
      setConnected(true);
      // Subscribe to all plug status topics
      client.subscribe("smart_plug/+/status", (err) => {
        if (err) console.error("[MQTT] Subscribe error:", err);
        else console.log("[MQTT] Subscribed to smart_plug/+/status");
      });
    });

    client.on("message", (topic, payload) => {
      // Parse topic: smart_plug/{uuid}/status
      const parts = topic.split("/");
      if (parts.length !== 3 || parts[0] !== "smart_plug" || parts[2] !== "status") return;

      const uuid = parts[1];
      try {
        const data = JSON.parse(payload.toString());
        const state: "on" | "off" = data.state === "on" ? "on" : "off";

        setPlugs((prev) => ({
          ...prev,
          [uuid]: {
            uuid,
            state,
            name: prev[uuid]?.name || getSavedName(uuid),
          },
        }));
      } catch (e) {
        console.error("[MQTT] Failed to parse message:", e);
      }
    });

    client.on("error", (err) => {
      console.error("[MQTT] Error:", err);
    });

    client.on("close", () => {
      console.log("[MQTT] Disconnected");
      setConnected(false);
    });

    return () => {
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

  return { plugs: Object.values(plugs), updateName, connected };
}
