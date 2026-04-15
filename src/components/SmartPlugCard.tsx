import { useState } from "react";
import { Pencil, Check, Power, Zap } from "lucide-react";

interface SmartPlugCardProps {
  uuid: string;
  state: "on" | "off";
  name: string;
  lastSeen: number | null;
  voltage: number | null;
  offline: boolean;
  onNameChange: (uuid: string, newName: string) => void;
  onCommand: (uuid: string, cmd: "on" | "off") => void;
}

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

const SmartPlugCard = ({
  uuid, state, name, lastSeen, voltage, offline, onNameChange, onCommand,
}: SmartPlugCardProps) => {
  const [editing, setEditing] = useState(false);
  const [editValue, setEditValue] = useState(name);
  const isOn = state === "on";

  const handleSave = () => {
    onNameChange(uuid, editValue.trim() || name);
    setEditing(false);
  };

  return (
    <div
      className={`relative rounded-2xl bg-card p-6 transition-all duration-300 ${offline ? "opacity-60" : ""}`}
      style={{
        boxShadow: isOn && !offline
          ? "var(--plug-card-shadow-on)"
          : "var(--plug-card-shadow)",
      }}
    >
      {/* Top status indicator bar */}
      <div
        className={`absolute top-0 left-1/2 -translate-x-1/2 h-1 w-16 rounded-b-full transition-colors duration-300 ${
          offline ? "bg-yellow-400" : isOn ? "bg-plug-on" : "bg-plug-off/30"
        }`}
      />

      {/* Offline badge */}
      {offline && (
        <div className="absolute top-3 right-3 text-[10px] font-semibold text-yellow-600 bg-yellow-100 px-2 py-0.5 rounded-full">
          오프라인
        </div>
      )}

      <div className="flex flex-col items-center gap-4 pt-2">
        {/* Power icon circle */}
        <div
          className={`flex items-center justify-center w-16 h-16 rounded-full transition-all duration-300 ${
            offline
              ? "bg-muted text-muted-foreground"
              : isOn
                ? "bg-plug-on-bg text-plug-on"
                : "bg-plug-off-bg text-plug-off"
          }`}
          style={
            isOn && !offline
              ? { boxShadow: "0 0 20px hsl(var(--plug-on-glow) / 0.25)" }
              : {}
          }
        >
          <Power className="w-7 h-7" strokeWidth={2.5} />
        </div>

        {/* Central info box */}
        <div
          className={`w-full rounded-xl border px-5 py-4 transition-colors duration-300 ${
            offline
              ? "border-yellow-300/30 bg-yellow-50/30"
              : isOn
                ? "border-plug-on/20 bg-plug-on-bg/50"
                : "border-border bg-muted/50"
          }`}
        >
          {/* Name row */}
          <div className="flex items-center gap-2 mb-1.5">
            {editing ? (
              <>
                <input
                  value={editValue}
                  onChange={(e) => setEditValue(e.target.value)}
                  onKeyDown={(e) => e.key === "Enter" && handleSave()}
                  autoFocus
                  className="flex-1 min-w-0 text-sm font-semibold bg-transparent border-b border-foreground/20 outline-none text-foreground py-0.5"
                />
                <button
                  onClick={handleSave}
                  className="text-plug-on hover:text-plug-on/80 transition-colors p-0.5"
                >
                  <Check className="w-3.5 h-3.5" />
                </button>
              </>
            ) : (
              <>
                <span className="text-sm font-semibold text-foreground truncate">
                  {name}
                </span>
                <button
                  onClick={() => {
                    setEditValue(name);
                    setEditing(true);
                  }}
                  className="text-muted-foreground hover:text-foreground transition-colors p-0.5 shrink-0"
                >
                  <Pencil className="w-3 h-3" />
                </button>
              </>
            )}
          </div>

          {/* UUID */}
          <p className="text-xs text-muted-foreground font-mono mb-2">
            {uuid}
          </p>

          {/* Status badge */}
          <div className="flex items-center gap-2 mb-2">
            <span
              className={`inline-block w-2 h-2 rounded-full transition-colors duration-300 ${
                offline ? "bg-yellow-400" : isOn ? "bg-plug-on animate-pulse" : "bg-plug-off/40"
              }`}
            />
            <span
              className={`text-xs font-semibold uppercase tracking-wider ${
                offline ? "text-yellow-600" : isOn ? "text-plug-on" : "text-plug-off"
              }`}
            >
              {offline ? "OFFLINE" : isOn ? "ON" : "OFF"}
            </span>
          </div>

          {/* Voltage */}
          {voltage !== null && (
            <div className="flex items-center gap-1.5 mb-2 text-xs text-muted-foreground">
              <Zap className="w-3 h-3" />
              <span>{voltage}V</span>
            </div>
          )}

          {/* Last seen */}
          <p className="text-[10px] text-muted-foreground/70">
            마지막 수신: {formatLastSeen(lastSeen)}
          </p>
        </div>

        {/* ON / OFF buttons */}
        <div className="flex gap-2 w-full">
          <button
            onClick={() => onCommand(uuid, "on")}
            className={`flex-1 py-2 rounded-lg text-xs font-bold uppercase tracking-wider transition-all duration-200 ${
              isOn && !offline
                ? "bg-plug-on text-white shadow-sm"
                : "bg-muted text-muted-foreground hover:bg-plug-on/10 hover:text-plug-on"
            }`}
          >
            ON
          </button>
          <button
            onClick={() => onCommand(uuid, "off")}
            className={`flex-1 py-2 rounded-lg text-xs font-bold uppercase tracking-wider transition-all duration-200 ${
              !isOn && !offline
                ? "bg-plug-off text-white shadow-sm"
                : "bg-muted text-muted-foreground hover:bg-plug-off/10 hover:text-plug-off"
            }`}
          >
            OFF
          </button>
        </div>
      </div>
    </div>
  );
};

export default SmartPlugCard;
