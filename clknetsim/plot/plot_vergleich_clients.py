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

offsets_chrony = load_offset("offset_20260702_chrony.log", max_seconds=600)
offsets_ntpd = load_offset("offset_20260702_ntpd.log", max_seconds=600)

zeiten_chrony = np.arange(len(offsets_chrony))
zeiten_ntpd = np.arange(len(offsets_ntpd))

plt.rcParams["font.size"] = 12
fig, ax = plt.subplots(figsize=(6, 4))

ax.plot(zeiten_chrony, offsets_chrony, label="chronyd", linewidth=1)
ax.plot(zeiten_ntpd, offsets_ntpd, label="ntpd", linewidth=1)
ax.axhline(0, color="red", linestyle="--", label="Referenz (0ms)")

ax.set_xlabel("Zeit (Sekunden)")
ax.set_ylabel("NTP Offset (ms)")
ax.legend(fontsize=10)
ax.grid(True, alpha=0.3)

plt.tight_layout()
ax.set_xlim(0, 600)
ax.set_ylim(-5, 105)
plt.savefig("diagramm_20260702_vergleich.svg", bbox_inches="tight")
print("Diagramm gespeichert als diagramm_20260702_vergleich.svg")
