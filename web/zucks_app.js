const TOKEN = "change-me-to-a-strong-random-token";
const API = "";

function headers() {
  return { "Authorization": "Bearer " + TOKEN, "Content-Type": "application/json" };
}

async function refresh() {
  try {
    const r = await fetch(API + "/api/status", { headers: headers() });
    const j = await r.json();
    document.getElementById("banned").textContent = j.banned_count;
    document.getElementById("version").textContent = j.version;
    document.getElementById("status").textContent = "Online";
    document.getElementById("status").className = "badge online";

    const b = await fetch(API + "/api/bans", { headers: headers() });
    const list = await b.json();
    const ul = document.getElementById("banList");
    ul.innerHTML = "";
    list.forEach(ip => {
      const li = document.createElement("li");
      li.textContent = ip;
      ul.appendChild(li);
    });
  } catch (e) {
    document.getElementById("status").textContent = "Offline";
    document.getElementById("status").className = "badge offline";
  }
}

async function unban() {
  const ip = document.getElementById("unbanInput").value.trim();
  if (!ip) return;
  await fetch(API + "/api/unban", {
    method: "POST", headers: headers(),
    body: JSON.stringify({ ip })
  });
  refresh();
}

setInterval(refresh, 3000);
refresh();