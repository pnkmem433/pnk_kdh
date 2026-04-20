import { Power, Wifi, WifiOff } from "lucide-react";
import SmartPlugCard from "@/components/SmartPlugCard";
import { useMqttPlugs } from "@/hooks/useMqttPlugs";

const statusConfig = {
  connected: { label: "Connected", color: "bg-green-500", textColor: "text-green-700", bgColor: "bg-green-50" },
  reconnecting: { label: "Reconnecting…", color: "bg-yellow-500 animate-pulse", textColor: "text-yellow-700", bgColor: "bg-yellow-50" },
  disconnected: { label: "Disconnected", color: "bg-red-500", textColor: "text-red-700", bgColor: "bg-red-50" },
} as const;

const Index = () => {
  const { plugs, updateName, sendCommand, connectionStatus } = useMqttPlugs();
  const status = statusConfig[connectionStatus];

  return (
    <div className="min-h-screen bg-background">
      <header className="border-b border-border bg-card">
        <div className="container max-w-6xl mx-auto px-4 py-5 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="flex items-center justify-center w-9 h-9 rounded-lg bg-primary text-primary-foreground">
              <Power className="w-5 h-5" />
            </div>
            <div>
              <h1 className="text-lg font-bold text-foreground leading-tight">Smart Plug Dashboard</h1>
              <p className="text-xs text-muted-foreground">{plugs.length} device{plugs.length !== 1 ? "s" : ""}</p>
            </div>
          </div>
          <div className={`flex items-center gap-2 px-3 py-1.5 rounded-full text-xs font-medium ${status.bgColor} ${status.textColor}`}>
            <span className={`w-2 h-2 rounded-full ${status.color}`} />
            {connectionStatus === "connected" ? <Wifi className="w-3.5 h-3.5" /> : <WifiOff className="w-3.5 h-3.5" />}
            {status.label}
          </div>
        </div>
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
          (() => {
            // Group plugs by location (text before " - "). Ungrouped plugs go under "기타".
            const groups = new Map<string, typeof plugs>();
            for (const plug of plugs) {
              const idx = plug.name.indexOf(" - ");
              const location = idx > 0 ? plug.name.slice(0, idx).trim() : "기타";
              if (!groups.has(location)) groups.set(location, []);
              groups.get(location)!.push(plug);
            }
            const sortedGroups = Array.from(groups.entries()).sort(([a], [b]) => {
              if (a === "기타") return 1;
              if (b === "기타") return -1;
              return a.localeCompare(b, "ko");
            });

            return (
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
                    <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
                      {items.map((plug) => (
                        <SmartPlugCard key={plug.uuid} plug={plug} onNameChange={updateName} onCommand={sendCommand} />
                      ))}
                    </div>
                  </section>
                ))}
              </div>
            );
          })()
        )}
      </main>
    </div>
  );
};

export default Index;
