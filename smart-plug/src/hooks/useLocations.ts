import { useState, useEffect, useCallback } from "react";

const LOCATIONS_KEY = "custom_locations";
const PLUG_LOCATION_KEY = "plug_locations";
const UNASSIGNED = "기타";

function readLocations(): string[] {
  try {
    const arr = JSON.parse(localStorage.getItem(LOCATIONS_KEY) || "[]");
    return Array.isArray(arr) ? arr.filter((x) => typeof x === "string") : [];
  } catch {
    return [];
  }
}

function writeLocations(list: string[]) {
  localStorage.setItem(LOCATIONS_KEY, JSON.stringify(list));
}

export function readPlugLocations(): Record<string, string> {
  try {
    const obj = JSON.parse(localStorage.getItem(PLUG_LOCATION_KEY) || "{}");
    return obj && typeof obj === "object" ? obj : {};
  } catch {
    return {};
  }
}

function writePlugLocations(map: Record<string, string>) {
  localStorage.setItem(PLUG_LOCATION_KEY, JSON.stringify(map));
}

export function getPlugLocation(uuid: string): string {
  return readPlugLocations()[uuid] || UNASSIGNED;
}

export function useLocations() {
  const [locations, setLocations] = useState<string[]>(() => readLocations());
  const [plugLocations, setPlugLocations] = useState<Record<string, string>>(() => readPlugLocations());

  // Sync across tabs / external writes
  useEffect(() => {
    const onStorage = (e: StorageEvent) => {
      if (e.key === LOCATIONS_KEY) setLocations(readLocations());
      if (e.key === PLUG_LOCATION_KEY) setPlugLocations(readPlugLocations());
    };
    window.addEventListener("storage", onStorage);
    return () => window.removeEventListener("storage", onStorage);
  }, []);

  const addLocation = useCallback((name: string) => {
    const trimmed = name.trim();
    if (!trimmed || trimmed === UNASSIGNED) return;
    setLocations((prev) => {
      if (prev.includes(trimmed)) return prev;
      const next = [...prev, trimmed];
      writeLocations(next);
      return next;
    });
  }, []);

  const removeLocation = useCallback((name: string) => {
    setLocations((prev) => {
      const next = prev.filter((l) => l !== name);
      writeLocations(next);
      return next;
    });
    // Unassign plugs from removed location
    setPlugLocations((prev) => {
      const next: Record<string, string> = {};
      let changed = false;
      for (const [uuid, loc] of Object.entries(prev)) {
        if (loc === name) { changed = true; continue; }
        next[uuid] = loc;
      }
      if (changed) writePlugLocations(next);
      return changed ? next : prev;
    });
  }, []);

  const setPlugLocation = useCallback((uuid: string, location: string) => {
    setPlugLocations((prev) => {
      const next = { ...prev };
      const trimmed = location.trim();
      if (!trimmed || trimmed === UNASSIGNED) {
        delete next[uuid];
      } else {
        next[uuid] = trimmed;
      }
      writePlugLocations(next);
      return next;
    });
  }, []);

  return { locations, plugLocations, addLocation, removeLocation, setPlugLocation };
}

export const UNASSIGNED_LOCATION = UNASSIGNED;
