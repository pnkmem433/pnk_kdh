import { Power } from "lucide-react";
import SmartPlugCard from "@/components/SmartPlugCard";
import { useMqttPlugs } from "@/hooks/useMqttPlugs";

const Index = () => {
  const { plugs, updateName, connected } = useMqttPlugs();

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="border-b border-border bg-card">
        <div className="container max-w-6xl mx-auto px-4 py-5 flex items-center gap-3">
          <div className="flex items-center justify-center w-9 h-9 rounded-lg bg-primary text-primary-foreground">
            <Power className="w-5 h-5" />
          </div>
          <div>
            <h1 className="text-lg font-bold text-foreground leading-tight">
              Smart Plug Dashboard
            </h1>
            <p className="text-xs text-muted-foreground">
              MQTT · {plugs.length} device{plugs.length !== 1 ? "s" : ""}
            </p>
          </div>
        </div>
      </header>

      {/* Grid */}
      <main className="container max-w-6xl mx-auto px-4 py-8">
        {plugs.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-24 text-muted-foreground">
            <Power className="w-12 h-12 mb-3 opacity-30" />
            <p className="text-sm">연결된 장치가 없습니다</p>
          </div>
        ) : (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
            {plugs.map((plug) => (
              <SmartPlugCard
                key={plug.uuid}
                uuid={plug.uuid}
                state={plug.state}
                name={plug.name}
                onNameChange={updateName}
              />
            ))}
          </div>
        )}
      </main>
    </div>
  );
};

export default Index;
