"""Plots the measurement results written by tests/measure.cpp.

usage:  python3 analyze.py <results_dir> <output_png>
"""
import csv
import collections
import math
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

res = sys.argv[1] if len(sys.argv) > 1 else "results"
out = sys.argv[2] if len(sys.argv) > 2 else "measurements.png"

# ---- palette (reference light palette, validated) ---------------------------
SURFACE, INK, INK2, MUTED = "#fcfcfb", "#0b0b0b", "#52514e", "#898781"
GRID, BASE = "#e1e0d9", "#c3c2b7"
MODE_COL = {0: "#2a78d6", 1: "#eb6834", 2: "#1baf7a"}          # FAT, CLEAN, ANALOG
MODE_NAME = {0: "FAT", 1: "CLEAN", 2: "ANALOG"}
RAMP = ["#86b6ef", "#3987e5", "#1c5cab", "#0d366b"]               # ordinal blue (250..700)
ORANGE = "#eb6834"

plt.rcParams.update({
    "font.family": "DejaVu Sans", "font.size": 9.5,
    "axes.facecolor": SURFACE, "figure.facecolor": SURFACE,
    "axes.edgecolor": BASE, "axes.labelcolor": INK2, "axes.titlecolor": INK,
    "axes.titlesize": 10.5, "axes.titleweight": "bold", "axes.titlelocation": "left",
    "xtick.color": MUTED, "ytick.color": MUTED, "grid.color": GRID, "grid.linewidth": 0.6,
    "legend.frameon": False, "legend.fontsize": 8.5, "lines.linewidth": 1.5,
})


def read(name):
    with open(f"{res}/{name}") as f:
        return list(csv.DictReader(f))


def style(ax):
    ax.grid(True, which="major")
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)


def end_label(ax, x, y, text, color):
    ax.annotate(text, (x, y), xytext=(4, 0), textcoords="offset points", va="center",
                fontsize=8.5, color=INK2, fontweight="bold")
    ax.plot([x], [y], "o", ms=4, color=color)


fig, axs = plt.subplots(2, 3, figsize=(15, 8.6), dpi=120)
fig.subplots_adjust(left=0.05, right=0.97, top=0.88, bottom=0.08, wspace=0.28, hspace=0.42)

# 1. static curves -------------------------------------------------------------
ax = axs[0][0]
st = collections.defaultdict(list)
for r in read("static.csv"):
    if r["mode"] == "2" and r["rev"] == "0":
        st[int(r["ratio"])].append((float(r["in_db"]), float(r["out_db"])))
names = ["4:1", "8:1", "12:1", "20:1", "ALL"]
ax.plot([-40, 6], [-40, 6], color=BASE, lw=1.0, zorder=1)
for k in range(5):
    pts = sorted(st[k])
    col = RAMP[k] if k < 4 else ORANGE
    ax.plot([p[0] for p in pts], [p[1] for p in pts], color=col, label=names[k], zorder=3)
ax.set_title("Static curves — ANALOG · BLACK, 1 kHz")
ax.set_xlabel("input level, dBFS (input knob 0 dB)")
ax.set_ylabel("output level, dBFS")
ax.set_xlim(-40, 6); ax.set_ylim(-40, 0)
ax.legend(loc="upper left", ncol=2)
style(ax)

# 2. attack ---------------------------------------------------------------------
ax = axs[0][1]
tr = collections.defaultdict(list)
for r in read("timing_traces.csv"):
    if r["mode"] == "2" and r["rev"] == "0" and r["which"] == "attack":
        tr[int(r["knob"])].append((float(r["t_ms"]), float(r["gr_db"])))
for i, k in enumerate([1, 3, 5, 7]):
    pts = sorted(tr[k])
    ax.plot([p[0] for p in pts], [p[1] for p in pts], color=RAMP[i], label=f"attack {k}")
ax.set_title("Attack — ANALOG · BLACK, +13 dB square burst")
ax.set_xlabel("time after step, ms"); ax.set_ylabel("gain reduction, dB")
ax.set_xlim(-0.1, 3.0)
ax.legend(loc="lower right")
style(ax)

# 3. program-dependent release ----------------------------------------------------
ax = axs[0][2]
rel = collections.defaultdict(list)
for r in read("timing_traces.csv"):
    if r["mode"] == "2" and r["rev"] == "0" and r["knob"] == "4" and r["which"].startswith("release"):
        rel[r["which"]].append((float(r["t_ms"]) / 1000.0, float(r["gr_db"])))
lab = {"release_short": ("after a 0.1 s hit", MODE_COL[0]), "release_long": ("after 2 s of compression", ORANGE)}
for key in ("release_short", "release_long"):
    pts = sorted(rel[key])
    ax.plot([p[0] for p in pts], [p[1] for p in pts], color=lab[key][1], label=lab[key][0])
ax.set_title("Release after a short vs a long hit — ANALOG")
ax.set_xlabel("time after the signal drops, s"); ax.set_ylabel("gain reduction, dB")
ax.set_xlim(-0.05, 1.5)
ax.legend(loc="upper right")
style(ax)

# 4/5. THD vs level ----------------------------------------------------------------
thd = read("thd.csv")
for rev, ax in ((0, axs[1][0]), (1, axs[1][1])):
    for mode in (0, 1, 2):
        pts = [(float(r["in_db"]), float(r["thd_pct"])) for r in thd
               if r["tag"] == "level1k" and int(r["mode"]) == mode and int(r["rev"]) == rev]
        pts.sort()
        if max(p[1] for p in pts) < 1e-4:
            ax.text(0.03, 0.06, f"{MODE_NAME[mode]}: no added harmonics (< 0.0001 %)",
                    transform=ax.transAxes, color=INK2, fontsize=8.5, fontweight="bold")
            ax.plot([], [], color=MODE_COL[mode], label=MODE_NAME[mode] + " (none)")
            continue
        ax.plot([p[0] for p in pts], [p[1] for p in pts], color=MODE_COL[mode], marker="o", ms=4.5,
                label=MODE_NAME[mode])
        end_label(ax, pts[-1][0], pts[-1][1], MODE_NAME[mode], MODE_COL[mode])
    ax.set_yscale("log")
    ax.set_ylim(0.01, 10)
    ax.set_xlim(-31, 4.5)
    ax.set_yticks([0.01, 0.1, 1, 10]); ax.set_yticklabels(["0.01 %", "0.1 %", "1 %", "10 %"])
    ax.set_title(("BLACK" if rev == 0 else "BLUE") + " — THD vs input, 1 kHz, 4:1 (compressing)")
    ax.set_xlabel("input level, dBFS (0 VU = −18)")
    ax.set_ylabel("THD (H2…H9)")
    ax.legend(loc="upper left")
    style(ax)

# 6. LF distortion at the fastest settings ------------------------------------------
ax = axs[1][2]
for mode, rev in ((2, 0), (2, 1), (0, 0), (1, 1)):
    pts = sorted((float(r["freq"]), float(r["thd_pct"])) for r in thd
                 if r["tag"] == "lf_fast" and int(r["mode"]) == mode and int(r["rev"]) == rev)
    col = MODE_COL[mode] if rev == 0 or mode != 2 else "#0f6e4c"
    name = MODE_NAME[mode] + (" · BLACK" if rev == 0 else " · BLUE")
    ax.plot([p[0] for p in pts], [p[1] for p in pts], color=col, marker="o", ms=4.5, label=name)
ax.text(0.03, 0.06, "CLEAN · BLACK: < 0.0001 % (hold detector)", transform=ax.transAxes,
        color=INK2, fontsize=8.5, fontweight="bold")
ax.set_xscale("log"); ax.set_yscale("log")
ax.set_xticks([40, 60, 100, 200]); ax.set_xticklabels(["40", "60", "100", "200"])
ax.set_yticks([0.1, 1, 10]); ax.set_yticklabels(["0.1 %", "1 %", "10 %"])
ax.set_ylim(0.1, 40)
ax.set_title("Bass distortion at attack 7 / release 7 (~9 dB GR)")
ax.set_xlabel("frequency, Hz"); ax.set_ylabel("THD (H2…H9)")
ax.legend(loc="upper right", ncol=2)
style(ax)

fig.suptitle("TwinFET — measurements of the DSP engine", x=0.05, ha="left", y=0.975,
             fontsize=14, fontweight="bold", color=INK)
fig.text(0.05, 0.935, "48 kHz host rate, 4× oversampling, input/output knobs at 0 dB unless noted. "
         "These are measurements of the plugin itself, not a comparison with a hardware unit.",
         color=INK2, fontsize=9.5)
fig.savefig(out)
print("wrote", out)

# ---- numbers for the README -----------------------------------------------------
import struct
def fr_at(path, freqs, fs=48000.0):
    data = open(path, "rb").read()
    x = struct.unpack(f"{len(data)//4}f", data)
    outv = []
    for f0 in freqs:
        re = sum(v * math.cos(2 * math.pi * f0 * n / fs) for n, v in enumerate(x[:8192]))
        im = sum(-v * math.sin(2 * math.pi * f0 * n / fs) for n, v in enumerate(x[:8192]))
        outv.append(20 * math.log10(max(1e-12, math.hypot(re, im))))
    return outv
freqs = [20, 50, 100, 1000, 10000, 20000]
for mode in range(3):
    for rev in range(2):
        v = fr_at(f"{res}/ir_{mode}_{rev}.f32", freqs)
        ref = v[3]
        print(MODE_NAME[mode], "BLACK" if rev == 0 else "BLUE", " ".join(f"{f}Hz:{a-ref:+.2f}" for f, a in zip(freqs, v)))
