export interface VersionProfile {
  id: string;
  displayName: string;
  magic: number[];
  headerSize: number;
  maxReassembledSize: number;
  preAuthTimeoutMs: number;
  fallback?: boolean;
  fields?: Record<string, unknown>;
}

export interface LiveMetrics {
  timestamp: number;
  activeConnections: number;
  packetsPerSecond: number;
  bytesPerSecond: number;
  detectedVersions: Record<string, number>;
  alerts: number;
  banned: number;
}

export interface EngineEvent {
  type: "metric" | "alert" | "ban" | "version-detected";
  payload: any;
  ts: number;
}