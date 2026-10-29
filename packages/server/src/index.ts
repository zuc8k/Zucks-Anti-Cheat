import Fastify from "fastify";
import cors from "@fastify/cors";
import jwt from "@fastify/jwt";
import websocket from "@fastify/websocket";
import { config } from "./config.js";
import { authRoutes } from "./routes/auth.routes.js";
import { metricsRoutes } from "./routes/metrics.routes.js";
import { bansRoutes } from "./routes/bans.routes.js";
import { versionsRoutes } from "./routes/versions.routes.js";
import { engineBridge } from "./services/engine-bridge.js";
import { registerWsHub } from "./plugins/ws.js";

const app = Fastify({ logger: true });

await app.register(cors, { origin: config.corsOrigin, credentials: true });
await app.register(jwt, { secret: config.jwtSecret });
await app.register(websocket);

// Health
app.get("/health", async () => ({ ok: true, version: "2.0.0" }));

// Routes
await app.register(authRoutes,     { prefix: "/api/auth" });
await app.register(metricsRoutes,  { prefix: "/api/metrics" });
await app.register(bansRoutes,     { prefix: "/api/bans" });
await app.register(versionsRoutes, { prefix: "/api/versions" });

// WebSocket hub (auth-required)
await registerWsHub(app);

// Start engine bridge (IPC to C++)
engineBridge.start();

app.listen({ port: config.port, host: "0.0.0.0" })
  .then((addr) => app.log.info(`Zucks server on ${addr}`))
  .catch((err) => { app.log.error(err); process.exit(1); });