import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MultipleLocator

def load_soll_offset(path):
    """Liest den Offset (Spalte 2) aus der erweiterten Logdatei (6 Spalten).
    Der Zeilenindex entspricht der Sekunde. Rueckgabe in Millisekunden."""
    vals = []
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) < 2:
                continue
            vals.append(float(parts[1]) * 1000)
    return np.array(vals)

def load_verlauf(path):
    """Liest die test_verlauf-Ausgabe (Zeit, angewendeter Offset in ms)."""
    t, v = [], []
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) < 2:
                continue
            t.append(float(parts[0]))
            v.append(float(parts[1]))
    return np.array(t), np.array(v)

soll_full = load_soll_offset("offset_20260702_extended_neu.log")
t_ist, ist = load_verlauf("verlauf_20260702_neu.log")

# Soll-Werte an denselben Zeitpunkten wie die Ist-Messung entnehmen
soll = np.array([soll_full[min(int(round(tt)), len(soll_full) - 1)] for tt in t_ist])
diff = ist - soll

plt.rcParams["font.size"] = 12
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(6, 3.2), sharex=True,
                                 gridspec_kw={"height_ratios": [2, 1]})

ax1.plot(t_ist, soll, label="Simulierter Offset (Soll)", linewidth=1)
ax1.plot(t_ist, ist, label="Von der Library angewendet (Ist)", linewidth=1, linestyle="--")
ax1.set_ylabel("Offset (ms)")
ax1.legend(fontsize=10)
ax1.grid(True, alpha=0.3)
ax1.yaxis.set_major_locator(MultipleLocator(50))

ax2.plot(t_ist, diff, color="grey", linewidth=1)
ax2.axhline(0, color="red", linestyle="--", linewidth=0.8)
ax2.set_ylabel("Differenz (ms)")
ax2.set_xlabel("Zeit (Sekunden)")
ax2.grid(True, alpha=0.3)
ax2.xaxis.set_major_locator(MultipleLocator(200))

plt.tight_layout()
plt.savefig("diagramm_20260702_verlauf_vergleich.svg", bbox_inches="tight")
print("Diagramm gespeichert als diagramm_20260702_verlauf_vergleich.svg")
