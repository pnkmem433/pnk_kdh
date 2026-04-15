import { useState, useEffect, useCallback } from "react";

export interface PlugData {
  uuid: string;
  state: "on" | "off";
  name: string;
}

/**
 * Mock MQTT hook — simulates smart_plug/{uuid}/status messages.
 * Replace internals with real MQTT.js client when ready.
 */
export function useMqttPlugs() {
  const [plugs, setPlugs] = useState<Record<string, PlugData>>({});

  // Load saved names from localStorage
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

  // Simulate incoming MQTT messages
  useEffect(() => {
    const mockDevices = [
      { uuid: "3C0F021851A4", state: "on" as const },
      { uuid: "A1B2C3D4E5F6", state: "off" as const },
      { uuid: "FF00AA11BB22", state: "on" as const },
      { uuid: "DE4D0C1F2A3B", state: "off" as const },
    ];

    const initial: Record<string, PlugData> = {};
    mockDevices.forEach((d) => {
      initial[d.uuid] = {
        uuid: d.uuid,
        state: d.state,
        name: getSavedName(d.uuid),
      };
    });
    setPlugs(initial);

    // Simulate random state changes every 8s
    const interval = setInterval(() => {
      setPlugs((prev) => {
        const uuids = Object.keys(prev);
        if (uuids.length === 0) return prev;
        const randomUuid = uuids[Math.floor(Math.random() * uuids.length)];
        return {
          ...prev,
          [randomUuid]: {
            ...prev[randomUuid],
            state: prev[randomUuid].state === "on" ? "off" : "on",
          },
        };
      });
    }, 8000);

    return () => clearInterval(interval);
  }, []);

  const updateName = useCallback((uuid: string, newName: string) => {
    saveName(uuid, newName);
    setPlugs((prev) => ({
      ...prev,
      [uuid]: { ...prev[uuid], name: newName },
    }));
  }, []);

  return { plugs: Object.values(plugs), updateName };
}
