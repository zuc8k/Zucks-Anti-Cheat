import { useEffect, useRef, useState } from "react";
import { useAuthStore } from "../store/useZucksStore";

export interface WsEvent {
  type: "metric" | "alert" | "ban" | "version-detected";
  payload: any;
  ts: number;
}

export function useZucksSocket() {
  const [events, setEvents] = useState<WsEvent[]>([]);
  const [connected, setConnected] = useState(false);
  const wsRef = useRef<WebSocket | null>(null);
  const token = useAuthStore(s => s.token);
  const BASE = (import.meta.env.VITE_WS ?? "ws://localhost:8080");

  useEffect(() => {
    if (!token) return;
    let closed = false;
    const connect = () => {
      const ws = new WebSocket(`${BASE}/ws?token=${encodeURIComponent(token)}`);
      wsRef.current = ws;
      ws.onopen = () => setConnected(true);
      ws.onclose = () => {
        setConnected(false);
        if (!closed) setTimeout(connect, 3000);
      };
      ws.onmessage = (e) => {
        try {
          const evt = JSON.parse(e.data) as WsEvent;
          setEvents(prev => [evt, ...prev].slice(0, 500));
        } catch {}
      };
    };
    connect();
    return () => { closed = true; wsRef.current?.close(); };
  }, [token]);

  const latestMetric = events.find(e => e.type === "metric")?.payload;
  return { events, latestMetric, connected };
}