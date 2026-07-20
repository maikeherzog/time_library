import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def load_offset(logfile):
    offsets_node2 = []
    with open(logfile, "r") as f:
        for line in f:
            parts = line.split()
            if len(parts) < 2:
                continue
            offsets_node2.append(float(parts[1]) * 1000)  # Sekunden -> Millisekunden
    return np.array(offsets_node2)

offsets_random = load_offset("offset_20260702_randomwalk_200000.log")
offsets_fest = load_offset("offset_20260702_fester_wert_200000.log")

zeiten_random = np.arange(len(offsets_random))
zeiten_fest = np.arange(len(offsets_fest))

plt.rcParams["font.size"] = 12
fig, ax = plt.subplots(figsize=(6, 4))

ax.plot(zeiten_random, offsets_random, label="Zufällige Drift", linewidth=1)
ax.plot(zeiten_fest, offsets_fest, label="Feste Frequenzabweichung", linewidth=1)
ax.axhline(0, color="red", linestyle="--", label="Referenz (0ms)")

ax.set_xlabel("Zeit (in 1000 Sekunden)")
ax.set_ylabel("NTP Offset (ms)")
ax.xaxis.set_major_formatter(FuncFormatter(lambda x, pos: f"{int(x/1000)}"))
ax.grid(True, alpha=0.3)

ax.legend(fontsize=10)
plt.tight_layout()
plt.savefig("diagramm_20260702_vergleich_drift_200000.svg", bbox_inches="tight")
print("Diagramm gespeichert als diagramm_20260702_vergleich_drift_200000.svg")
