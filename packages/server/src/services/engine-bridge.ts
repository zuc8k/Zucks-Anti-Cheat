import net from "node:net";
import { EventEmitter } from "node:events";
import { config } from "../config.js";
import type { EngineEvent } from "../types.js";

class EngineBridge extends EventEmitter {
  private socket?: net.Socket;
  private buf = "";
  private lastMetrics: EngineEvent | null = null;

  start() {
    this.connect();
  }

  private connect() {
    const sock = net.createConnection({ path: config.ipcSocket });
    sock.on("connect", () => console.log("[Zucks] Engine IPC connected"));
    sock.on("data", (chunk) => this.onData(chunk));
    sock.on("close", () => {
      console.warn("[Zucks] Engine IPC closed — retrying in 3s");
      this.socket = undefined;
      setTimeout(() => this.connect(), 3000);
    });
    sock.on("error", () => {});
    this.socket = sock;
  }

  private onData(chunk: Buffer) {
    this.buf += chunk.toString("utf8");
    let idx: number;
    while ((idx = this.buf.indexOf("\n")) >= 0) {
      const line = this.buf.slice(0, idx).trim();
      this.buf = this.buf.slice(idx + 1);
      if (!line) continue;
      try {
        const evt = JSON.parse(line) as EngineEvent;
        if (evt.type === "metric") this.lastMetrics = evt;
        this.emit("event", evt);
      } catch { /* ignore malformed */ }
    }
  }

  onEvent(cb: (e: EngineEvent) => void) {
    this.on("event", cb);
    return () => this.off("event", cb);
  }

  sendCommand(cmd: object) {
    if (!this.socket) return false;
    this.socket.write(JSON.stringify(cmd) + "\n");
    return true;
  }

  latest() { return this.lastMetrics; }
}

export const engineBridge = new EngineBridge();