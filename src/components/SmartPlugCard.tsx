import { useState } from "react";
import { Pencil, Check, Power, Globe, Wifi, ChevronDown, ChevronUp, X } from "lucide-react";
import type { PlugData } from "@/hooks/useMqttPlugs";
import { UNASSIGNED_LOCATION } from "@/hooks/useLocations";

function formatLastSeen(ts: number | null): string {
  if (!ts) return "—";
  const d = new Date(ts);
  const yyyy = d.getFullYear();
  const mm = String(d.getMonth() + 1).padStart(2, "0");
  const dd = String(d.getDate()).padStart(2, "0");
  const hour = d.getHours();
  const min = String(d.getMinutes()).padStart(2, "0");
  const ampm = hour < 12 ? "오전" : "오후";
  const h12 = hour % 12 || 12;
  return `${yyyy}-${mm}-${dd} ${ampm} ${h12}:${min}`;
}

interface Props {
  plug: PlugData;
  location: string;
  locations: string[];
  onNameChange: (uuid: string, newName: string) => void;
  onLocationChange: (uuid: string, location: string) => void;
  onAddLocation: (name: string) => void;
  onCommand: (uuid: string, cmd: "on" | "off") => void;
}

const SmartPlugCard = ({ plug, location, locations, onNameChange, onLocationChange, onAddLocation, onCommand }: Props) => {
  const { uuid, name, state, webserver, lastSeen, offline, ssid, ipAddress, extraFields } = plug;
  const [editing, setEditing] = useState(false);
  const [editName, setEditName] = useState(name);
  const [editLocation, setEditLocation] = useState(location);
  const [showLocPicker, setShowLocPicker] = useState(false);
  const [newLocInput, setNewLocInput] = useState("");
  const [showExtra, setShowExtra] = useState(false);
  const isOn = state === "on";
  // Firmware 자동 판별: webserver 필드가 MQTT로 오면 Tasmota, 없으면 자체제작
  const isTasmota = webserver !== null;
  // 자체제작 펌웨어는 WiFi가 하드코딩되어 있음 (MQTT로 SSID 안 옴)
  const displaySsid = ssid ?? (isTasmota ? null : "CC-Retail");
  const extraKeys = Object.keys(extraFields);

  const handleSave = () => {
    onNameChange(uuid, editName.trim() || name);
    onLocationChange(uuid, editLocation);
    setEditing(false);
    setShowLocPicker(false);
  };

  const handleAddNewLoc = () => {
    const v = newLocInput.trim();
    if (!v) return;
    onAddLocation(v);
    setEditLocation(v);
    setNewLocInput("");
    setShowLocPicker(false);
  };

  return (
    <div
      className={`relative rounded-2xl bg-card p-6 transition-all duration-300 ${offline ? "opacity-60" : ""}`}
      style={{ boxShadow: isOn && !offline ? "var(--plug-card-shadow-on)" : "var(--plug-card-shadow)" }}
    >
      {/* Top status bar */}
      <div className={`absolute top-0 left-1/2 -translate-x-1/2 h-1 w-16 rounded-b-full transition-colors duration-300 ${
        offline ? "bg-yellow-400" : isOn ? "bg-plug-on" : "bg-plug-off/30"
      }`} />

      {offline && (
        <div className="absolute top-3 right-3 text-[10px] font-semibold text-yellow-600 bg-yellow-100 px-2 py-0.5 rounded-full">
          오프라인
        </div>
      )}

      <div className="flex flex-col items-center gap-4 pt-2">
        {/* Power icon */}
        <div
          className={`flex items-center justify-center w-16 h-16 rounded-full transition-all duration-300 ${
            offline ? "bg-muted text-muted-foreground"
              : isOn ? "bg-plug-on-bg text-plug-on" : "bg-plug-off-bg text-plug-off"
          }`}
          style={isOn && !offline ? { boxShadow: "0 0 20px hsl(var(--plug-on-glow) / 0.25)" } : {}}
        >
          <Power className="w-7 h-7" strokeWidth={2.5} />
        </div>

        {/* Info box */}
        <div className={`w-full rounded-xl border px-5 py-4 transition-colors duration-300 ${
          offline ? "border-yellow-300/30 bg-yellow-50/30"
            : isOn ? "border-plug-on/20 bg-plug-on-bg/50" : "border-border bg-muted/50"
        }`}>
          {/* Name + Location editor */}
          <div className="flex items-start gap-2 mb-1.5">
            {editing ? (
              <div className="flex-1 min-w-0 space-y-2">
                <div className="relative">
                  <button
                    type="button"
                    onClick={() => setShowLocPicker((v) => !v)}
                    className="w-full flex items-center justify-between text-[11px] font-medium px-2 py-1 rounded border border-border bg-background text-foreground hover:bg-muted transition-colors"
                  >
                    <span className="truncate">📍 {editLocation || UNASSIGNED_LOCATION}</span>
                    <ChevronDown className="w-3 h-3 shrink-0" />
                  </button>
                  {showLocPicker && (
                    <div className="absolute z-10 left-0 right-0 mt-1 rounded-md border border-border bg-popover shadow-lg p-1 max-h-56 overflow-auto">
                      <button
                        type="button"
                        onClick={() => { setEditLocation(UNASSIGNED_LOCATION); setShowLocPicker(false); }}
                        className="w-full text-left text-[11px] px-2 py-1 rounded hover:bg-muted text-muted-foreground"
                      >
                        {UNASSIGNED_LOCATION} (지정 안함)
                      </button>
                      {locations.map((loc) => (
                        <button
                          key={loc}
                          type="button"
                          onClick={() => { setEditLocation(loc); setShowLocPicker(false); }}
                          className={`w-full text-left text-[11px] px-2 py-1 rounded hover:bg-muted ${editLocation === loc ? "bg-muted font-semibold" : ""}`}
                        >
                          {loc}
                        </button>
                      ))}
                      <div className="flex items-center gap-1 border-t border-border mt-1 pt-1">
                        <input
                          value={newLocInput}
                          onChange={(e) => setNewLocInput(e.target.value)}
                          onKeyDown={(e) => { if (e.key === "Enter") { e.preventDefault(); handleAddNewLoc(); } }}
                          placeholder="새 장소 추가"
                          className="flex-1 min-w-0 text-[11px] px-2 py-1 rounded bg-background border border-border outline-none focus:border-foreground/40"
                        />
                        <button type="button" onClick={handleAddNewLoc} className="text-[11px] px-2 py-1 rounded bg-primary text-primary-foreground hover:opacity-90">
                          추가
                        </button>
                      </div>
                    </div>
                  )}
                </div>
                <div className="flex items-center gap-1">
                  <input
                    value={editName}
                    onChange={(e) => setEditName(e.target.value)}
                    onKeyDown={(e) => e.key === "Enter" && handleSave()}
                    placeholder="이름/설명"
                    autoFocus
                    className="flex-1 min-w-0 text-sm font-semibold bg-transparent border-b border-foreground/20 outline-none text-foreground py-0.5 break-all"
                  />
                  <button onClick={handleSave} className="text-plug-on hover:text-plug-on/80 transition-colors p-0.5 shrink-0">
                    <Check className="w-3.5 h-3.5" />
                  </button>
                  <button onClick={() => { setEditing(false); setShowLocPicker(false); }} className="text-muted-foreground hover:text-foreground transition-colors p-0.5 shrink-0">
                    <X className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>
            ) : (
              <>
                <div className="flex-1 min-w-0">
                  {location && location !== UNASSIGNED_LOCATION && (
                    <div className="text-[10px] font-medium text-muted-foreground mb-0.5">📍 {location}</div>
                  )}
                  <span className="text-sm font-semibold text-foreground break-all leading-snug">{name}</span>
                </div>
                <button onClick={() => { setEditName(name); setEditLocation(location); setEditing(true); }} className="text-muted-foreground hover:text-foreground transition-colors p-0.5 shrink-0 mt-0.5">
                  <Pencil className="w-3 h-3" />
                </button>
              </>
            )}
          </div>

          {/* UUID */}
          <p className="text-xs text-muted-foreground font-mono mb-2">{uuid}</p>

          {/* State + Webserver badges */}
          <div className="flex items-center gap-2 mb-2 flex-wrap">
            <div className="flex items-center gap-1.5">
              <span className={`inline-block w-2 h-2 rounded-full transition-colors duration-300 ${
                offline ? "bg-yellow-400" : isOn ? "bg-plug-on animate-pulse" : "bg-plug-off/40"
              }`} />
              <span className={`text-xs font-semibold uppercase tracking-wider ${
                offline ? "text-yellow-600" : isOn ? "text-plug-on" : "text-plug-off"
              }`}>
                {state === null ? "UNKNOWN" : isOn ? "ON" : "OFF"}
              </span>
            </div>
            {isTasmota ? (
              <span className="text-[10px] font-medium px-2 py-0.5 rounded-full bg-secondary text-secondary-foreground">
                WEB {webserver}
              </span>
            ) : (
              <span className="text-[10px] font-medium px-2 py-0.5 rounded-full bg-primary/10 text-primary">
                자체제작
              </span>
            )}
          </div>

          {/* ON/OFF buttons */}
          <div className="flex gap-2 mb-3">
            <button
              onClick={() => onCommand(uuid, "on")}
              disabled={offline}
              className={`flex-1 py-1.5 rounded-lg text-xs font-semibold transition-all duration-200 ${
                isOn
                  ? "bg-plug-on text-white shadow-sm"
                  : "bg-muted text-muted-foreground hover:bg-plug-on/10 hover:text-plug-on"
              } disabled:opacity-40 disabled:cursor-not-allowed`}
            >
              ON
            </button>
            <button
              onClick={() => onCommand(uuid, "off")}
              disabled={offline}
              className={`flex-1 py-1.5 rounded-lg text-xs font-semibold transition-all duration-200 ${
                !isOn && state !== null
                  ? "bg-plug-off text-white shadow-sm"
                  : "bg-muted text-muted-foreground hover:bg-plug-off/10 hover:text-plug-off"
              } disabled:opacity-40 disabled:cursor-not-allowed`}
            >
              OFF
            </button>
          </div>

          {/* Network info */}
          {(displaySsid || ipAddress) && (
            <div className="flex items-center gap-3 text-[10px] text-muted-foreground/70 mb-2 flex-wrap">
              {displaySsid && (
                <div className="flex items-center gap-1">
                  <Wifi className="w-3 h-3 shrink-0" /><span>{displaySsid}</span>
                </div>
              )}
              {ipAddress && (
                <div className="flex items-center gap-1">
                  <Globe className="w-3 h-3 shrink-0" /><span>{ipAddress}</span>
                </div>
              )}
            </div>
          )}

          {/* Last seen */}
          <p className="text-[10px] text-muted-foreground/70">
            마지막 수신: {formatLastSeen(lastSeen)}
          </p>

          {/* Extra fields (collapsible) */}
          {extraKeys.length > 0 && (
            <div className="mt-2 border-t border-border/50 pt-2">
              <button
                onClick={() => setShowExtra(!showExtra)}
                className="flex items-center gap-1 text-[10px] text-muted-foreground hover:text-foreground transition-colors"
              >
                {showExtra ? <ChevronUp className="w-3 h-3" /> : <ChevronDown className="w-3 h-3" />}
                추가 정보 ({extraKeys.length})
              </button>
              {showExtra && (
                <div className="mt-1.5 space-y-0.5">
                  {extraKeys.map((key) => (
                    <div key={key} className="flex items-start gap-2 text-[10px] text-muted-foreground/70 font-mono">
                      <span className="shrink-0 text-muted-foreground">{key}:</span>
                      <span className="break-all">{JSON.stringify(extraFields[key])}</span>
                    </div>
                  ))}
                </div>
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

export default SmartPlugCard;
