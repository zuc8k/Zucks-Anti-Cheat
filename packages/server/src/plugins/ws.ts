import type { FastifyInstance } from "fastify";
import { engineBridge } from "../services/engine-bridge.js";

const clients = new Set<any>();

export async function registerWsHub(app: FastifyInstance) {
  app.get("/ws", { websocket: true }, (socket, req) => {
    try {
      const token = new URL(req.url!, "http://x").searchParams.get("token");
      if (!token) return socket.close(4001, "missing token");
      app.jwt.verify(token);
    } catch {
      return socket.close(4001, "unauthorized");
    }
    clients.add(socket);
    socket.on("close", () => clients.delete(socket));
    socket.on("error", () => clients.delete(socket));
  });

  engineBridge.onEvent((evt) => {
    const msg = JSON.stringify(evt);
    for (const s of clients) {
      try { s.send(msg); } catch { clients.delete(s); }
    }
  });

  app.log.info("WebSocket hub ready at /ws");
}