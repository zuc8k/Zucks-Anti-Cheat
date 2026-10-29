import { useAuthStore } from "../store/useZucksStore";

const BASE = import.meta.env.VITE_API ?? "http://localhost:8080";

export async function api<T>(path: string, init: RequestInit = {}): Promise<T> {
  const token = useAuthStore.getState().token;
  const res = await fetch(`${BASE}${path}`, {
    ...init,
    headers: {
      "Content-Type": "application/json",
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
      ...(init.headers ?? {})
    }
  });
  if (res.status === 401) useAuthStore.getState().logout();
  if (!res.ok) throw new Error(await res.text());
  return res.json() as Promise<T>;
}