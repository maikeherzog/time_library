import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MultipleLocator

def load_zeitvergleich(path):
    """Liest die test_zeitvergleich-Ausgabe (Zeit, echte Zeit, manipulierte Zeit)."""
    t, real, manip = [], [], []
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) < 3:
                continue
            t.append(float(parts[0]))
            real.append(float(parts[1]))
            manip.append(float(parts[2]))
    return np.array(t), np.array(real), np.array(manip)

t, real, manip = load_zeitvergleich("zeitvergleich_gross.log")
diff = manip - real

plt.rcParams["font.size"] = 12
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(6, 3.2), sharex=True,
                                 gridspec_kw={"height_ratios": [3, 1]})

ax1.plot(t, real, label="Echte Zeit", linewidth=1)
ax1.plot(t, manip, label="Manipulierte Zeit", linewidth=1, linestyle="--")
ax1.set_ylabel("Gemessene Zeit (Sekunden)")
ax1.legend(fontsize=10)
ax1.grid(True, alpha=0.3)
ax1.yaxis.set_major_locator(MultipleLocator(300))

ax2.plot(t, diff, color="grey", linewidth=1)
ax2.axhline(0, color="red", linestyle="--", linewidth=0.8)
ax2.set_ylabel("Differenz (s)")
ax2.set_xlabel("Zeit (Sekunden)")
ax2.set_yticks([0, 10])
ax2.grid(True, alpha=0.3)
ax2.xaxis.set_major_locator(MultipleLocator(200))

plt.tight_layout()
plt.savefig("diagramm_20260702_zeit_gross.svg", bbox_inches="tight")
print("Diagramm gespeichert als diagramm_20260702_zeit_gross.svg")
