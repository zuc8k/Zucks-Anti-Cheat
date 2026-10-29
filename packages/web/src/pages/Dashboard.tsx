import { Activity, ShieldAlert, Ban, Radio } from "lucide-react";
import { useZucksSocket } from "../lib/ws";
import { MetricCard } from "../components/MetricCard";
import { AreaChart, Area, XAxis, YAxis, Tooltip, ResponsiveContainer } from "recharts";
import { useState, useEffect } from "react";

export default function Dashboard() {
  const { latestMetric, events, connected } = useZucksSocket();
  const [history, setHistory] = useState<{ t: string; pps: number; bps: number }[]>([]);

  useEffect(() => {
    if (!latestMetric) return;
    setHistory(h => [...h, {
      t: new Date().toLocaleTimeString(),
      pps: latestMetric.packetsPerSecond,
      bps: Math.round(latestMetric.bytesPerSecond / 1024)
    }].slice(-60));
  }, [latestMetric]);

  const alerts = events.filter(e => e.type === "alert").length;

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-bold text-zucks-text">Dashboard</h1>
        <span className={`text-xs px-3 py-1 rounded-full border ${
          connected ? "border-zucks-ok/40 text-zucks-ok" : "border-zucks-danger/40 text-zucks-danger"
        }`}>{connected ? "● Live" : "○ Disconnected"}</span>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
        <MetricCard label="Active Conn." value={latestMetric?.activeConnections ?? 0} icon={Radio} />
        <MetricCard label="Packets / sec" value={latestMetric?.packetsPerSecond ?? 0} icon={Activity} tone="ok" />
        <MetricCard label="Alerts" value={alerts} icon={ShieldAlert} tone="warn" />
        <MetricCard label="Banned IPs" value={latestMetric?.banned ?? 0} icon={Ban} tone="danger" />
      </div>

      <div className="bg-zucks-panel border border-zucks-border rounded-xl p-5">
        <h2 className="text-sm uppercase tracking-wider text-zucks-muted mb-4">
          Traffic (last 60 samples)
        </h2>
        <ResponsiveContainer width="100%" height={260}>
          <AreaChart data={history}>
            <defs>
              <linearGradient id="ppsGrad" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor="#22d3ee" stopOpacity={0.6} />
                <stop offset="100%" stopColor="#22d3ee" stopOpacity={0} />
              </linearGradient>
            </defs>
            <XAxis dataKey="t" stroke="#475569" fontSize={11} />
            <YAxis stroke="#475569" fontSize={11} />
            <Tooltip contentStyle={{ background: "#111823", border: "1px solid #1f2937" }} />
            <Area type="monotone" dataKey="pps" stroke="#22d3ee" fill="url(#ppsGrad)" />
          </AreaChart>
        </ResponsiveContainer>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        <div className="bg-zucks-panel border border-zucks-border rounded-xl p-5">
          <h2 className="text-sm uppercase tracking-wider text-zucks-muted mb-3">Detected Versions</h2>
          <ul className="space-y-2 font-mono text-sm">
            {Object.entries(latestMetric?.detectedVersions ?? {}).map(([id, n]) => (
              <li key={id} className="flex justify-between text-zucks-text">
                <span>{id}</span><span className="text-zucks-accent">{n as number}</span>
              </li>
            ))}
            {!latestMetric?.detectedVersions && <li className="text-zucks-muted">No clients yet</li>}
          </ul>
        </div>
        <div className="bg-zucks-panel border border-zucks-border rounded-xl p-5">
          <h2 className="text-sm uppercase tracking-wider text-zucks-muted mb-3">Recent Alerts</h2>
          <ul className="space-y-1 text-sm font-mono max-h-48 overflow-auto">
            {events.filter(e => e.type === "alert").slice(0, 8).map((e, i) => (
              <li key={i} className="text-zucks-warn">
                {new Date(e.ts).toLocaleTimeString()} — {e.payload.ip} — {e.payload.reason}
              </li>
            ))}
            {alerts === 0 && <li className="text-zucks-muted">No alerts 🎉</li>}
          </ul>
        </div>
      </div>
    </div>
  );
}