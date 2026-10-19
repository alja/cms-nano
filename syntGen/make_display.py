#!/usr/bin/env python3
"""Build a self-contained HTML event display from jets.txt + particles.txt.

Charged particles are propagated as helices in a uniform 3.8 T field inside
the page (JS), so only momenta are embedded.
"""
import argparse
import json
from collections import defaultdict


def read_table(fname):
    cols, rows = None, []
    with open(fname) as f:
        for line in f:
            if line.startswith("#"):
                names = line[1:].split()
                if names and names[0] == "iev":
                    cols = names
                continue
            if line.strip():
                rows.append(dict(zip(cols, (float(x) for x in line.split()))))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--jets", default="jets.txt")
    ap.add_argument("--particles", default="particles.txt")
    ap.add_argument("-o", "--output", default="event_display.html")
    args = ap.parse_args()

    events = defaultdict(lambda: {"jets": [], "particles": []})
    for j in read_table(args.jets):
        ev = events[int(j["iev"])]
        ev["event"] = int(j["event"])
        ev["pv"] = [j["pvx"] / 100, j["pvy"] / 100, j["pvz"] / 100]  # cm -> m
        raw = 1 - j["rawFactor"]
        ev["jets"].append({
            "i": int(j["ijet"]), "pt": round(j["pt"], 2), "ptRaw": round(j["pt"] * raw, 2),
            "eta": round(j["eta"], 4), "phi": round(j["phi"], 4),
            "m": round(j["mass"] * raw, 2), "nCon": int(j["nConstituents"]),
            "flav": int(j.get("partonFlavour", 0)),
        })
    for p in read_table(args.particles):
        events[int(p["iev"])]["particles"].append([
            int(p["ijet"]), int(p["pdgId"]), int(p["charge"]),
            round(p["px"], 4), round(p["py"], 4), round(p["pz"], 4)])

    data = [dict(iev=k, **v) for k, v in sorted(events.items())]
    html = TEMPLATE.replace("/*DATA*/null", json.dumps(data, separators=(",", ":")))
    with open(args.output, "w") as f:
        f.write(html)
    print("wrote %s (%d events)" % (args.output, len(data)))


TEMPLATE = r"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Synthetic Jet Tracks</title>
<style>
:root {
  color-scheme: light;
  --surface-0: #f4f4f2; --surface-1: #fcfcfb; --border: #dddcd6;
  --text-primary: #0b0b0b; --text-secondary: #52514e; --text-muted: #85847e;
  --detector: #c9c8c1; --neutral: #9a998f;
  --neg: #2a78d6; --pos: #eb6834;
  --jet-fill: rgba(120,118,110,.10); --jet-line: #a9a89f;
}
@media (prefers-color-scheme: dark) {
  :root:where(:not([data-theme="light"])) {
    color-scheme: dark;
    --surface-0: #111110; --surface-1: #1a1a19; --border: #33332f;
    --text-primary: #ffffff; --text-secondary: #c3c2b7; --text-muted: #8c8b83;
    --detector: #45443f; --neutral: #77766e;
    --neg: #3987e5; --pos: #d95926;
    --jet-fill: rgba(200,198,190,.08); --jet-line: #5c5b55;
  }
}
:root[data-theme="dark"] {
  color-scheme: dark;
  --surface-0: #111110; --surface-1: #1a1a19; --border: #33332f;
  --text-primary: #ffffff; --text-secondary: #c3c2b7; --text-muted: #8c8b83;
  --detector: #45443f; --neutral: #77766e;
  --neg: #3987e5; --pos: #d95926;
  --jet-fill: rgba(200,198,190,.08); --jet-line: #5c5b55;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--surface-0); color: var(--text-primary);
  font: 14px/1.45 -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
main { max-width: 1240px; margin: 0 auto; padding: 20px 16px 40px; }
h1 { font-size: 18px; margin: 0 0 2px; font-weight: 600; }
.sub { color: var(--text-secondary); margin: 0 0 14px; }
.controls { display: flex; flex-wrap: wrap; gap: 10px 18px; align-items: center; margin-bottom: 14px; }
.controls label { color: var(--text-secondary); display: flex; gap: 6px; align-items: center; }
select, button { font: inherit; color: var(--text-primary); background: var(--surface-1);
  border: 1px solid var(--border); border-radius: 6px; padding: 4px 8px; }
button { cursor: pointer; }
.legend { display: flex; gap: 14px; flex-wrap: wrap; color: var(--text-secondary); font-size: 13px; }
.legend span { display: inline-flex; align-items: center; gap: 6px; }
.sw { width: 18px; height: 0; border-top: 2px solid; display: inline-block; }
.grid { display: grid; grid-template-columns: minmax(0, 1fr) minmax(0, 1.25fr); gap: 14px; }
@media (max-width: 860px) { .grid { grid-template-columns: 1fr; } }
.panel { background: var(--surface-1); border: 1px solid var(--border); border-radius: 10px; padding: 10px; }
.panel h2 { font-size: 13px; font-weight: 600; margin: 0 0 6px; color: var(--text-secondary); }
svg { width: 100%; height: auto; display: block; }
.trk { fill: none; stroke-width: 1.6; stroke-linecap: round; }
.trk.hit { stroke-width: 10; stroke: transparent; cursor: pointer; }
.trk.dim { opacity: .18; }
.det { fill: none; stroke: var(--detector); stroke-width: 1; }
.axis { fill: var(--text-muted); }
.jetlbl { fill: var(--text-secondary); }
#tip { position: fixed; pointer-events: none; background: var(--surface-1); color: var(--text-primary);
  border: 1px solid var(--border); border-radius: 8px; padding: 6px 9px; font-size: 12px;
  box-shadow: 0 4px 14px rgba(0,0,0,.15); display: none; white-space: nowrap; }
#tip b { font-weight: 600; }
#tip .m { color: var(--text-secondary); }
table { border-collapse: collapse; width: 100%; font-size: 12.5px; font-variant-numeric: tabular-nums; }
th, td { text-align: right; padding: 4px 8px; border-bottom: 1px solid var(--border); }
th { color: var(--text-secondary); font-weight: 600; }
th:first-child, td:first-child { text-align: left; }
tr.sel td { background: var(--jet-fill); }
tbody tr { cursor: pointer; }
.note { color: var(--text-muted); font-size: 12px; margin-top: 10px; }
</style>
</head>
<body>
<main>
  <h1>Synthetic jet tracks</h1>
  <p class="sub">Pythia 8 constituents generated for CMS NanoAOD jets · helices in a uniform 3.8 T field · not reconstructed tracks</p>
  <div class="controls">
    <label>Event <select id="ev"></select></label>
    <button id="prev" aria-label="Previous event">‹</button><button id="next" aria-label="Next event">›</button>
    <label><input type="checkbox" id="neutral" checked> neutrals</label>
    <label>min track p<sub>T</sub> <select id="ptmin">
      <option value="0">0</option><option value="0.5" selected>0.5 GeV</option>
      <option value="1">1 GeV</option><option value="2">2 GeV</option></select></label>
    <div class="legend">
      <span><i class="sw" style="border-color:var(--pos)"></i>positive</span>
      <span><i class="sw" style="border-color:var(--neg)"></i>negative</span>
      <span><i class="sw" style="border-color:var(--neutral);border-top-style:dashed"></i>neutral</span>
      <span><i class="sw" style="border-color:var(--jet-line);border-top-width:8px;opacity:.6"></i>NanoAOD jet (R=0.4)</span>
    </div>
  </div>
  <div class="grid">
    <div class="panel"><h2>Transverse view (x–y)</h2><svg id="xy" viewBox="-2.25 -2.05 4.5 4.1"></svg></div>
    <div class="panel"><h2>Side view (z–ρ, ρ signed by initial p<sub>y</sub>)</h2><svg id="rz" viewBox="-3.2 -1.75 6.4 3.5"></svg></div>
  </div>
  <div class="panel" style="margin-top:14px">
    <h2>Jets in this event — NanoAOD vs. generated (hover a row to highlight)</h2>
    <table><thead><tr><th>jet</th><th>p<sub>T</sub> raw</th><th>η</th><th>φ</th><th>mass</th><th>NanoAOD n const.</th>
      <th>gen. particles</th><th>gen. charged</th><th>flavour</th></tr></thead><tbody id="jt"></tbody></table>
    <p class="note">Tracker volume drawn at ρ = 1.1 m, |z| = 2.8 m; ECAL front at ρ = 1.29 m. Neutrals drawn as straight lines to the ECAL.
    Energy loss and multiple scattering are ignored.</p>
  </div>
</main>
<div id="tip" role="tooltip"></div>
<script>
const DATA = /*DATA*/null;
const B = 3.8, R_TRK = 1.1, Z_TRK = 2.8, R_ECAL = 1.29, Z_ECAL = 3.0;
const NAMES = {22:"γ",11:"e",13:"μ",211:"π",321:"K",310:"K⁰S",130:"K⁰L",2212:"p",2112:"n",3122:"Λ",
  3112:"Σ",3222:"Σ",3312:"Ξ",3322:"Ξ⁰",3334:"Ω",111:"π⁰"};
function pname(id, q) {
  const a = Math.abs(id), n = NAMES[a] || ("pdg " + id);
  if (a === 22 || a === 310 || a === 130 || a === 111) return n;
  if (a === 2112 || a === 3122 || a === 3322) return id < 0 ? "anti-" + n : n;
  return n + (q > 0 ? "⁺" : q < 0 ? "⁻" : "⁰");
}
const NS = "http://www.w3.org/2000/svg";
const el = (t, a, p) => { const e = document.createElementNS(NS, t); for (const k in a) e.setAttribute(k, a[k]); if (p) p.appendChild(e); return e; };

// Step a charged particle through a uniform Bz field. Returns [x,y,z] points (m).
// Positive charges bend clockwise in x-y for B along +z.
function helix(p, pv) {
  const [, , q, px, py, pz] = p;
  const pt = Math.hypot(px, py);
  let phi = Math.atan2(py, px), x = pv[0], y = pv[1], z = pv[2];
  const ds = 0.01, kappa = -0.3 * q * B / pt, dzds = pz / pt;   // dphi per metre of transverse path
  const pts = [[x, y, z]];
  const sMax = Math.min(6, 2 * Math.PI / Math.abs(kappa));
  for (let s = 0; s < sMax; s += ds) {
    phi += kappa * ds;
    x += Math.cos(phi) * ds; y += Math.sin(phi) * ds; z += dzds * ds;
    pts.push([x, y, z]);
    if (Math.hypot(x, y) > R_TRK || Math.abs(z) > Z_TRK) break;
  }
  return pts;
}
function straight(p, pv) {
  const [, , , px, py, pz] = p, pt = Math.hypot(px, py);
  const t = Math.min(R_ECAL / pt, Z_ECAL / Math.max(Math.abs(pz), 1e-9));
  return [[pv[0], pv[1], pv[2]], [pv[0] + px * t, pv[1] + py * t, pv[2] + pz * t]];
}

const xy = document.getElementById("xy"), rz = document.getElementById("rz");
const tip = document.getElementById("tip");
let cur = 0, hlJet = null;

function detector() {
  xy.innerHTML = ""; rz.innerHTML = "";
  // y up in physics coordinates: flip with a group transform.
  const gxy = el("g", {transform: "scale(1,-1)"}, xy);
  el("circle", {class: "det", cx: 0, cy: 0, r: R_TRK, "vector-effect": "non-scaling-stroke"}, gxy);
  el("circle", {class: "det", cx: 0, cy: 0, r: R_ECAL, "stroke-dasharray": "3 3", "vector-effect": "non-scaling-stroke"}, gxy);
  const grz = el("g", {transform: "scale(1,-1)"}, rz);
  for (const s of [1, -1]) {
    el("rect", {class: "det", x: -Z_TRK, y: s > 0 ? 0 : -R_TRK, width: 2 * Z_TRK, height: R_TRK, "vector-effect": "non-scaling-stroke"}, grz);
  }
  el("line", {class: "det", x1: -Z_ECAL, y1: R_ECAL, x2: Z_ECAL, y2: R_ECAL, "stroke-dasharray": "3 3", "vector-effect": "non-scaling-stroke"}, grz);
  el("line", {class: "det", x1: -Z_ECAL, y1: -R_ECAL, x2: Z_ECAL, y2: -R_ECAL, "stroke-dasharray": "3 3", "vector-effect": "non-scaling-stroke"}, grz);
  el("text", {class: "axis", x: 1.33, y: 0.05, "font-size": 0.09}, xy).textContent = "x";
  el("text", {class: "axis", x: 0.03, y: -1.33, "font-size": 0.09}, xy).textContent = "y";
  el("text", {class: "axis", x: 2.9, y: 0.12, "font-size": 0.09}, rz).textContent = "z";
  el("text", {class: "axis", x: -3.1, y: -1.62, "font-size": 0.09}, rz).textContent = "tracker |z| < 2.8 m";
  return [gxy, grz];
}

function draw() {
  const ev = DATA[cur];
  document.getElementById("ev").value = cur;
  const [gxy, grz] = detector();
  const showN = document.getElementById("neutral").checked;
  const ptmin = +document.getElementById("ptmin").value;
  const jxy = el("g", {}, gxy), jrz = el("g", {}, grz), txy = el("g", {}, gxy), trz = el("g", {}, grz);
  const hits = el("g", {}, gxy), hitsRz = el("g", {}, grz);

  const placed = [];
  // Jet cones: phi +- 0.4 wedge in x-y; theta range from eta +- 0.4 in z-rho.
  for (const j of ev.jets) {
    const L = 1.6, a0 = j.phi - 0.4, a1 = j.phi + 0.4;
    el("path", {d: `M0,0 L${L*Math.cos(a0)},${L*Math.sin(a0)} A${L},${L} 0 0 1 ${L*Math.cos(a1)},${L*Math.sin(a1)} Z`,
      fill: "var(--jet-fill)", stroke: "var(--jet-line)", "stroke-width": 0.004, "data-jet": j.i, class: "jet"}, jxy);
    const s = Math.sin(j.phi) >= 0 ? 1 : -1;
    const th = e => 2 * Math.atan(Math.exp(-e));
    const ta = th(j.eta + 0.4), tb = th(j.eta - 0.4), Lr = 1.6;
    el("path", {d: `M0,0 L${Lr*Math.cos(ta)},${s*Lr*Math.sin(ta)} L${Lr*Math.cos(tb)},${s*Lr*Math.sin(tb)} Z`,
      fill: "var(--jet-fill)", stroke: "var(--jet-line)", "stroke-width": 0.004, "data-jet": j.i, class: "jet"}, jrz);
    // Stagger labels of jets that are close in phi.
    let rl = 1.68;
    while (placed.some(([a, r]) => r === rl && Math.abs(Math.atan2(Math.sin(a - j.phi), Math.cos(a - j.phi))) < 0.3)) rl += 0.14;
    placed.push([j.phi, rl]);
    const lx = rl * Math.cos(j.phi), ly = -rl * Math.sin(j.phi);
    const t = el("text", {class: "jetlbl", x: lx, y: ly, "font-size": 0.085, "text-anchor": lx >= 0 ? "start" : "end",
      "dominant-baseline": "middle"}, xy);
    t.textContent = `J${j.i} ${j.ptRaw.toFixed(0)} GeV`;
  }

  for (const p of ev.particles) {
    const [ij, id, q, px, py, pz] = p;
    const pt = Math.hypot(px, py);
    if (q === 0 && !showN) continue;
    if (q !== 0 && pt < ptmin) continue;
    const pts = q === 0 ? straight(p, ev.pv) : helix(p, ev.pv);
    const color = q > 0 ? "var(--pos)" : q < 0 ? "var(--neg)" : "var(--neutral)";
    const dxy = "M" + pts.map(v => v[0].toFixed(4) + "," + v[1].toFixed(4)).join("L");
    const sgn = py >= 0 ? 1 : -1;
    const drz = "M" + pts.map(v => v[2].toFixed(4) + "," + (sgn * Math.hypot(v[0] - ev.pv[0], v[1] - ev.pv[1])).toFixed(4)).join("L");
    const attrs = {class: "trk", stroke: color, "vector-effect": "non-scaling-stroke", "data-jet": ij};
    if (q === 0) attrs["stroke-dasharray"] = "4 3";
    el("path", {d: dxy, ...attrs}, txy);
    el("path", {d: drz, ...attrs}, trz);
    const eta = Math.asinh(pz / pt), phi = Math.atan2(py, px);
    const info = {ij, name: pname(id, q), id, pt, eta, phi, e: Math.hypot(pt, pz)};
    for (const [g, d] of [[hits, dxy], [hitsRz, drz]]) {
      const h = el("path", {d, class: "trk hit", "vector-effect": "non-scaling-stroke"}, g);
      h.addEventListener("pointermove", e => showTip(e, info));
      h.addEventListener("pointerleave", () => { tip.style.display = "none"; highlight(null); });
    }
  }

  const tb = document.getElementById("jt"); tb.innerHTML = "";
  for (const j of ev.jets) {
    const ps = ev.particles.filter(p => p[0] === j.i), nch = ps.filter(p => p[2] !== 0).length;
    const fl = j.flav === 21 ? "g" : j.flav === 0 ? "–" : Math.abs(j.flav) === 5 ? "b" : Math.abs(j.flav) === 4 ? "c" : "light q";
    const tr = document.createElement("tr");
    tr.innerHTML = `<td>J${j.i}</td><td>${j.ptRaw.toFixed(1)}</td><td>${j.eta.toFixed(2)}</td><td>${j.phi.toFixed(2)}</td>` +
      `<td>${j.m.toFixed(1)}</td><td>${j.nCon}</td><td>${ps.length}</td><td>${nch}</td><td>${fl}</td>`;
    tr.addEventListener("pointerenter", () => highlight(j.i));
    tr.addEventListener("pointerleave", () => highlight(null));
    tb.appendChild(tr);
  }
}

function highlight(ij) {
  for (const e of document.querySelectorAll(".trk:not(.hit)"))
    e.classList.toggle("dim", ij !== null && +e.dataset.jet !== ij);
  for (const r of document.querySelectorAll("#jt tr"))
    r.classList.toggle("sel", ij !== null && r.firstChild.textContent === "J" + ij);
}

function showTip(e, i) {
  tip.innerHTML = `<b>${i.name}</b> <span class="m">(pdg ${i.id}) · jet J${i.ij}</span><br>` +
    `p<sub>T</sub> ${i.pt.toFixed(2)} GeV · E ${i.e.toFixed(2)} GeV<br>η ${i.eta.toFixed(2)} · φ ${i.phi.toFixed(2)}`;
  tip.style.display = "block";
  const w = tip.offsetWidth;
  tip.style.left = Math.min(e.clientX + 14, innerWidth - w - 8) + "px";
  tip.style.top = (e.clientY + 14) + "px";
  highlight(i.ij);
}

const sel = document.getElementById("ev");
DATA.forEach((ev, k) => { const o = document.createElement("option"); o.value = k;
  o.textContent = `${ev.iev} (evt ${ev.event}, ${ev.jets.length} jets)`; sel.appendChild(o); });
sel.addEventListener("change", () => { cur = +sel.value; draw(); });
document.getElementById("prev").onclick = () => { cur = (cur + DATA.length - 1) % DATA.length; draw(); };
document.getElementById("next").onclick = () => { cur = (cur + 1) % DATA.length; draw(); };
document.getElementById("neutral").onchange = draw;
document.getElementById("ptmin").onchange = draw;
draw();
</script>
</body>
</html>
"""

if __name__ == "__main__":
    main()
