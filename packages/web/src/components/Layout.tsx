import { NavLink, useNavigate } from "react-router-dom";
import { LayoutDashboard, Ban, Boxes, ScrollText, LogOut } from "lucide-react";
import { useAuthStore } from "../store/useZucksStore";

const links = [
  { to: "/",         label: "Dashboard", icon: LayoutDashboard },
  { to: "/bans",     label: "Bans",      icon: Ban },
  { to: "/versions", label: "Versions",  icon: Boxes },
  { to: "/logs",     label: "Logs",      icon: ScrollText }
];

export default function Layout({ children }: { children: React.ReactNode }) {
  const logout = useAuthStore(s => s.logout);
  const nav = useNavigate();
  const user = useAuthStore(s => s.user);
  return (
    <div className="min-h-screen flex bg-zucks-bg">
      <aside className="w-60 bg-zucks-panel border-r border-zucks-border flex flex-col">
        <div className="p-5 border-b border-zucks-border">
          <div className="flex items-center gap-2">
            <span className="text-2xl">🛡️</span>
            <div>
              <div className="font-bold text-zucks-text">Zucks</div>
              <div className="text-[10px] text-zucks-muted tracking-widest">ANTI-CHEAT v2</div>
            </div>
          </div>
        </div>
        <nav className="flex-1 p-3 space-y-1">
          {links.map(({ to, label, icon: Icon }) => (
            <NavLink key={to} to={to} end
              className={({ isActive }) => `flex items-center gap-3 px-3 py-2 rounded-lg text-sm transition ${
                isActive ? "bg-zucks-accent/10 text-zucks-accent" : "text-zucks-muted hover:text-zucks-text"
              }`}>
              <Icon size={16} /> {label}
            </NavLink>
          ))}
        </nav>
        <div className="p-3 border-t border-zucks-border">
          <div className="text-xs text-zucks-muted mb-2 px-2">👤 {user?.username ?? "admin"}</div>
          <button onClick={() => { logout(); nav("/login"); }}
            className="w-full flex items-center gap-2 px-3 py-2 rounded-lg text-sm text-zucks-muted hover:text-zucks-danger hover:bg-zucks-danger/10">
            <LogOut size={14} /> Sign out
          </button>
        </div>
      </aside>
      <main className="flex-1 p-8 overflow-auto">{children}</main>
    </div>
  );
}