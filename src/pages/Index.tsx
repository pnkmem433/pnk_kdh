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
      {/* Header */}
      <header className="border-b border-border bg-card">
        <div className="container max-w-6xl mx-auto px-4 py-5 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="flex items-center justify-center w-9 h-9 rounded-lg bg-primary text-primary-foreground">
              <Power className="w-5 h-5" />
            </div>
            <div>
              <h1 className="text-lg font-bold text-foreground leading-tight">
                Smart Plug Dashboard
              </h1>
              <p className="text-xs text-muted-foreground">
                {plugs.length} device{plugs.length !== 1 ? "s" : ""}
              </p>
            </div>
          </div>

          {/* Connection status badge */}
          <div className={`flex items-center gap-2 px-3 py-1.5 rounded-full text-xs font-medium ${status.bgColor} ${status.textColor}`}>
            <span className={`w-2 h-2 rounded-full ${status.color}`} />
            {connectionStatus === "connected" ? (
              <Wifi className="w-3.5 h-3.5" />
            ) : (
              <WifiOff className="w-3.5 h-3.5" />
            )}
            {status.label}
          </div>
        </div>
      </header>

      {/* Grid */}
      <main className="container max-w-6xl mx-auto px-4 py-8">
        {plugs.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-24 text-muted-foreground">
            <Power className="w-12 h-12 mb-3 opacity-30" />
            <p className="text-sm">연결된 장치가 없습니다</p>
            {connectionStatus === "disconnected" && (
              <p className="text-xs mt-1 text-red-500">MQTT 브로커에 연결할 수 없습니다</p>
            )}
            {connectionStatus === "reconnecting" && (
              <p className="text-xs mt-1 text-yellow-600">브로커에 재연결 시도 중…</p>
            )}
          </div>
        ) : (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
            {plugs.map((plug) => (
              <SmartPlugCard
                key={plug.uuid}
                uuid={plug.uuid}
                state={plug.state}
                name={plug.name}
                lastSeen={plug.lastSeen}
                voltage={plug.voltage}
                offline={plug.offline}
                ssid={plug.ssid}
                ipAddress={plug.ipAddress}
                onNameChange={updateName}
                onCommand={sendCommand}
              />
            ))}
          </div>
        )}
      </main>
    </div>
  );
};

export default Index;
