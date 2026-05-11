import { useState } from "react";
import { Power, Wifi, WifiOff, Plus, X } from "lucide-react";
import SmartPlugCard from "@/components/SmartPlugCard";
import { useMqttPlugs } from "@/hooks/useMqttPlugs";
import { useLocations, UNASSIGNED_LOCATION } from "@/hooks/useLocations";

const statusConfig = {
  connected: { label: "Connected", color: "bg-green-500", textColor: "text-green-700", bgColor: "bg-green-50" },
  reconnecting: { label: "Reconnecting…", color: "bg-yellow-500 animate-pulse", textColor: "text-yellow-700", bgColor: "bg-yellow-50" },
  disconnected: { label: "Disconnected", color: "bg-red-500", textColor: "text-red-700", bgColor: "bg-red-50" },
} as const;

const Index = () => {
  const { plugs, updateName, sendCommand, sendOta, connectionStatus } = useMqttPlugs();
  const { locations, plugLocations, addLocation, removeLocation, setPlugLocation } = useLocations();
  const status = statusConfig[connectionStatus];
  const [showAddLoc, setShowAddLoc] = useState(false);
  const [newLocName, setNewLocName] = useState("");

  const handleAddLocation = () => {
    const v = newLocName.trim();
    if (!v) return;
    addLocation(v);
    setNewLocName("");
    setShowAddLoc(false);
  };

  // Group plugs by stored location (fallback: UNASSIGNED)
  const groups = new Map<string, typeof plugs>();
  // Ensure all known custom locations show up even if empty
  for (const loc of locations) groups.set(loc, []);
  for (const plug of plugs) {
    const loc = plugLocations[plug.uuid] || UNASSIGNED_LOCATION;
    if (!groups.has(loc)) groups.set(loc, []);
    groups.get(loc)!.push(plug);
  }
  const sortedGroups = Array.from(groups.entries()).sort(([a], [b]) => {
    if (a === UNASSIGNED_LOCATION) return 1;
    if (b === UNASSIGNED_LOCATION) return -1;
    return a.localeCompare(b, "ko");
  });

  return (
    <div className="min-h-screen bg-background">
      <header className="border-b border-border bg-card">
        <div className="container max-w-6xl mx-auto px-4 py-5 flex items-center justify-between gap-3 flex-wrap">
          <div className="flex items-center gap-3">
            <div className="flex items-center justify-center w-9 h-9 rounded-lg bg-primary text-primary-foreground">
              <Power className="w-5 h-5" />
            </div>
            <div>
              <h1 className="text-lg font-bold text-foreground leading-tight">Smart Plug Dashboard</h1>
              <p className="text-xs text-muted-foreground">{plugs.length} device{plugs.length !== 1 ? "s" : ""} · {locations.length} 장소</p>
            </div>
          </div>
          <div className="flex items-center gap-2">
            <button
              onClick={() => setShowAddLoc((v) => !v)}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-full text-xs font-medium bg-primary text-primary-foreground hover:opacity-90 transition-opacity"
            >
              <Plus className="w-3.5 h-3.5" /> 장소 추가
            </button>
            <div className={`flex items-center gap-2 px-3 py-1.5 rounded-full text-xs font-medium ${status.bgColor} ${status.textColor}`}>
              <span className={`w-2 h-2 rounded-full ${status.color}`} />
              {connectionStatus === "connected" ? <Wifi className="w-3.5 h-3.5" /> : <WifiOff className="w-3.5 h-3.5" />}
              {status.label}
            </div>
          </div>
        </div>

        {showAddLoc && (
          <div className="container max-w-6xl mx-auto px-4 pb-4">
            <div className="flex items-center gap-2 p-3 rounded-lg border border-border bg-muted/40">
              <input
                value={newLocName}
                onChange={(e) => setNewLocName(e.target.value)}
                onKeyDown={(e) => e.key === "Enter" && handleAddLocation()}
                placeholder="장소 이름 (예: 2층 연구소, 907호)"
                autoFocus
                className="flex-1 text-sm bg-background border border-border rounded px-3 py-1.5 outline-none focus:border-foreground/40 text-foreground"
              />
              <button onClick={handleAddLocation} className="text-xs px-3 py-1.5 rounded bg-primary text-primary-foreground hover:opacity-90 font-medium">
                추가
              </button>
              <button onClick={() => { setShowAddLoc(false); setNewLocName(""); }} className="text-xs px-2 py-1.5 rounded text-muted-foreground hover:text-foreground">
                취소
              </button>
            </div>
            {locations.length > 0 && (
              <div className="flex flex-wrap gap-2 mt-3">
                {locations.map((loc) => (
                  <span key={loc} className="inline-flex items-center gap-1 text-xs bg-secondary text-secondary-foreground px-2.5 py-1 rounded-full">
                    📍 {loc}
                    <button
                      onClick={() => { if (confirm(`"${loc}" 장소를 삭제할까요? (해당 플러그는 "${UNASSIGNED_LOCATION}"으로 이동)`)) removeLocation(loc); }}
                      className="hover:text-destructive transition-colors"
                      aria-label={`${loc} 삭제`}
                    >
                      <X className="w-3 h-3" />
                    </button>
                  </span>
                ))}
              </div>
            )}
          </div>
        )}
      </header>

      <main className="container max-w-6xl mx-auto px-4 py-8">
        {plugs.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-24 text-muted-foreground">
            <Power className="w-12 h-12 mb-3 opacity-30" />
            <p className="text-sm">연결된 장치가 없습니다</p>
            {connectionStatus === "disconnected" && <p className="text-xs mt-1 text-red-500">MQTT 브로커에 연결할 수 없습니다</p>}
            {connectionStatus === "reconnecting" && <p className="text-xs mt-1 text-yellow-600">브로커에 재연결 시도 중…</p>}
          </div>
        ) : (
          <div className="space-y-8">
            {sortedGroups.map(([location, items]) => (
              <section key={location}>
                <div className="flex items-center gap-3 mb-4">
                  <h2 className="text-base font-bold text-foreground">{location}</h2>
                  <span className="text-xs text-muted-foreground px-2 py-0.5 rounded-full bg-muted">
                    {items.length}
                  </span>
                  <div className="flex-1 h-px bg-border" />
                </div>
                {items.length === 0 ? (
                  <div className="text-xs text-muted-foreground italic px-1">이 장소에 등록된 플러그가 없습니다. 플러그 카드의 ✏️ 아이콘을 눌러 장소를 지정하세요.</div>
                ) : (
                  <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
                    {items.map((plug) => (
                      <SmartPlugCard
                        key={plug.uuid}
                        plug={plug}
                        location={plugLocations[plug.uuid] || UNASSIGNED_LOCATION}
                        locations={locations}
                        onNameChange={updateName}
                        onLocationChange={setPlugLocation}
                        onAddLocation={addLocation}
                        onCommand={sendCommand}
                      />
                    ))}
                  </div>
                )}
              </section>
            ))}
          </div>
        )}
      </main>
    </div>
  );
};

export default Index;
