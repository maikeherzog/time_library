import matplotlib.pyplot as plt
import numpy as np

def load_offset(logfile, max_seconds=None):
    offsets_node2 = []
    with open(logfile, "r") as f:
        for line in f:
            parts = line.split()
            if len(parts) < 2:
                continue
            offsets_node2.append(float(parts[1]) * 1000)  # Sekunden -> Millisekunden
    offsets_node2 = np.array(offsets_node2)
    if max_seconds is not None:
        offsets_node2 = offsets_node2[:max_seconds]
    return offsets_node2

offsets = load_offset("offset_20260702_zittern.log", max_seconds=600)
zeiten = np.arange(len(offsets))

plt.rcParams["font.size"] = 12
fig, ax = plt.subplots(figsize=(6, 4))

ax.plot(zeiten, offsets, label="ntpd", linewidth=1)
ax.axhline(0, color="red", linestyle="--", label="Referenz (0ms)")

ax.set_xlabel("Zeit (Sekunden)")
ax.set_ylabel("NTP Offset (ms)")
ax.legend(fontsize=10)
ax.grid(True, alpha=0.3)

plt.tight_layout()
ax.set_xlim(0, 600)
plt.savefig("diagramm_20260702_zittern.svg", bbox_inches="tight")
print("Diagramm gespeichert als diagramm_20260702_zittern.svg")
