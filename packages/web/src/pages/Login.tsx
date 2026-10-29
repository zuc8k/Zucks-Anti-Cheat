import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { api } from "../lib/api";
import { useAuthStore } from "../store/useZucksStore";

export default function Login() {
  const [u, setU] = useState("admin");
  const [p, setP] = useState("");
  const [err, setErr] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const login = useAuthStore(s => s.login);
  const nav = useNavigate();

  const submit = async (e: React.FormEvent) => {
    e.preventDefault(); setErr(null); setLoading(true);
    try {
      const res = await api<{ token: string; user: any }>("/api/auth/login", {
        method: "POST", body: JSON.stringify({ username: u, password: p })
      });
      login(res.token, res.user);
      nav("/");
    } catch { setErr("Invalid credentials"); }
    finally { setLoading(false); }
  };

  return (
    <div className="min-h-screen flex items-center justify-center bg-zucks-bg">
      <form onSubmit={submit}
        className="w-full max-w-sm bg-zucks-panel border border-zucks-border rounded-2xl p-8 shadow-2xl">
        <div className="text-center mb-8">
          <div className="text-4xl mb-2">🛡️</div>
          <h1 className="text-2xl font-bold text-zucks-text">Zucks-Anti-Cheat</h1>
          <p className="text-zucks-muted text-sm mt-1">Control Plane v2.0</p>
        </div>
        {err && <div className="mb-4 text-zucks-danger text-sm text-center">{err}</div>}
        <label className="block text-zucks-muted text-xs mb-1">USERNAME</label>
        <input value={u} onChange={e => setU(e.target.value)}
          className="w-full mb-4 px-3 py-2 rounded-lg bg-zucks-bg border border-zucks-border text-zucks-text outline-none focus:border-zucks-accent" />
        <label className="block text-zucks-muted text-xs mb-1">PASSWORD</label>
        <input type="password" value={p} onChange={e => setP(e.target.value)}
          className="w-full mb-6 px-3 py-2 rounded-lg bg-zucks-bg border border-zucks-border text-zucks-text outline-none focus:border-zucks-accent" />
        <button disabled={loading}
          className="w-full py-2.5 rounded-lg bg-zucks-accent text-zucks-bg font-bold hover:brightness-110 disabled:opacity-50">
          {loading ? "Signing in..." : "Sign In"}
        </button>
      </form>
    </div>
  );
}