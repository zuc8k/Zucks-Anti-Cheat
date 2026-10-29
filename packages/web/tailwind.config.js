export default {
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        zucks: {
          bg:     "#0a0e14",
          panel:  "#111823",
          border: "#1f2937",
          accent: "#22d3ee",
          danger: "#f43f5e",
          ok:     "#22c55e",
          warn:   "#f59e0b",
          text:   "#e5e7eb",
          muted:  "#94a3b8"
        }
      },
      fontFamily: { mono: ["JetBrains Mono", "monospace"] }
    }
  },
  plugins: []
};