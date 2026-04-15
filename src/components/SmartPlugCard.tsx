import { useState } from "react";
import { Pencil, Check, Power } from "lucide-react";

interface SmartPlugCardProps {
  uuid: string;
  state: "on" | "off";
  name: string;
  onNameChange: (uuid: string, newName: string) => void;
}

const SmartPlugCard = ({ uuid, state, name, onNameChange }: SmartPlugCardProps) => {
  const [editing, setEditing] = useState(false);
  const [editValue, setEditValue] = useState(name);
  const isOn = state === "on";

  const handleSave = () => {
    onNameChange(uuid, editValue.trim() || name);
    setEditing(false);
  };

  return (
    <div
      className="relative rounded-2xl bg-card p-6 transition-all duration-300"
      style={{
        boxShadow: isOn
          ? "var(--plug-card-shadow-on)"
          : "var(--plug-card-shadow)",
      }}
    >
      {/* Top status indicator bar */}
      <div
        className={`absolute top-0 left-1/2 -translate-x-1/2 h-1 w-16 rounded-b-full transition-colors duration-300 ${
          isOn ? "bg-plug-on" : "bg-plug-off/30"
        }`}
      />

      {/* Plug visual */}
      <div className="flex flex-col items-center gap-4 pt-2">
        {/* Power icon circle */}
        <div
          className={`flex items-center justify-center w-16 h-16 rounded-full transition-all duration-300 ${
            isOn
              ? "bg-plug-on-bg text-plug-on"
              : "bg-plug-off-bg text-plug-off"
          }`}
          style={
            isOn
              ? { boxShadow: "0 0 20px hsl(var(--plug-on-glow) / 0.25)" }
              : {}
          }
        >
          <Power className="w-7 h-7" strokeWidth={2.5} />
        </div>

        {/* Central info box */}
        <div
          className={`w-full rounded-xl border px-5 py-4 transition-colors duration-300 ${
            isOn
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
          <p className="text-xs text-muted-foreground font-mono mb-3">
            {uuid}
          </p>

          {/* Status badge */}
          <div className="flex items-center gap-2">
            <span
              className={`inline-block w-2 h-2 rounded-full transition-colors duration-300 ${
                isOn ? "bg-plug-on animate-pulse" : "bg-plug-off/40"
              }`}
            />
            <span
              className={`text-xs font-semibold uppercase tracking-wider ${
                isOn ? "text-plug-on" : "text-plug-off"
              }`}
            >
              {isOn ? "ON" : "OFF"}
            </span>
          </div>
        </div>
      </div>
    </div>
  );
};

export default SmartPlugCard;
