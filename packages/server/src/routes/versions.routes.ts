import type { FastifyInstance } from "fastify";
import fs from "node:fs";
import path from "node:path";
import { config } from "../config.js";

export async function versionsRoutes(app: FastifyInstance) {
  app.get("/", async () => {
    const dir = config.profilesDir;
    if (!fs.existsSync(dir)) return [];
    return fs.readdirSync(dir)
      .filter(f => f.endsWith(".json"))
      .map(f => {
        try {
          return JSON.parse(fs.readFileSync(path.join(dir, f), "utf8"));
        } catch { return null; }
      })
      .filter(Boolean);
  });
}