import { clsx } from "clsx";
import type { LucideIcon } from "lucide-react";

interface Props {
  label: string;
  value: string | number;
  icon: LucideIcon;
  tone?: "default" | "ok" | "warn" | "danger";
  hint?: string;
}

export function MetricCard({ label, value, icon: Icon, tone = "default", hint }: Props) {
  const tones = {
    default: "text-zucks-accent border-zucks-border",
    ok:      "text-zucks-ok border-zucks-ok/30",
    warn:    "text-zucks-warn border-zucks-warn/30",
    danger:  "text-zucks-danger border-zucks-danger/30"
  } as const;
  return (
    <div className={clsx("bg-zucks-panel border rounded-xl p-5", tones[tone])}>
      <div className="flex items-center justify-between">
        <span className="text-zucks-muted text-xs uppercase tracking-wider">{label}</span>
        <Icon size={18} className="opacity-70" />
      </div>
      <div className="text-3xl font-bold mt-2 font-mono">{value}</div>
      {hint && <div className="text-zucks-muted text-xs mt-1">{hint}</div>}
    </div>
  );
}