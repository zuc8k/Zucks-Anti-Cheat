export const config = {
  port: Number(process.env.ZUCKS_PORT ?? 8080),
  jwtSecret: process.env.ZUCKS_JWT_SECRET ?? "dev-change-me",
  ipcSocket: process.env.ZUCKS_IPC ?? "\\\\.\\pipe\\zucks_engine",
  profilesDir: process.env.ZUCKS_PROFILES ?? "../profiles",
  corsOrigin: process.env.ZUCKS_CORS ?? "http://localhost:5173",
} as const;