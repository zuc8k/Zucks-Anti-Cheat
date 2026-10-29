import type { FastifyInstance } from "fastify";
import { engineBridge } from "../services/engine-bridge.js";

export async function metricsRoutes(app: FastifyInstance) {
  app.get("/latest", async () => engineBridge.latest() ?? { type: "metric", payload: null });
  app.get("/profiles", async () => {
    // In real impl: read from ProfileLoader exposed via IPC
    return engineBridge.latest()?.payload?.detectedVersions ?? {};
  });
}