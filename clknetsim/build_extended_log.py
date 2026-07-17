import sys

FREQ_SCALE = 65536.0
PHI = 15e-6  # NTP-Standard: feste Wachstumsrate fuer maxerror/esterror (Sekunden pro Sekunde)


def read_offsets(path):
    offsets = []
    with open(path) as f:
        for line in f:
            cols = line.split()
            if len(cols) >= 2:
                offsets.append(float(cols[1]))
    return offsets


def read_peerstats(path):
    points = []
    with open(path) as f:
        for line in f:
            cols = line.split()
            if len(cols) < 7:
                continue
            t     = float(cols[1])
            delay = float(cols[5])
            disp  = float(cols[6])
            points.append((t, disp + delay / 2.0, disp))
    points.sort(key=lambda p: p[0])
    return points


def read_loopstats(path):
    points = []
    with open(path) as f:
        for line in f:
            cols = line.split()
            if len(cols) < 7:
                continue
            t        = float(cols[1])
            freq_ppm = float(cols[3])
            constant = float(cols[6])
            points.append((t, freq_ppm * FREQ_SCALE, constant))
    points.sort(key=lambda p: p[0])
    return points


def interp2(points, t):
    """Interpoliert zwei Werte (Spalte 2 und 3 der Tupel) linear zum Zeitpunkt t."""
    if not points:
        return 0.0, 0.0
    if t <= points[0][0]:
        return points[0][1], points[0][2]
    if t >= points[-1][0]:
        return points[-1][1], points[-1][2]
    for i in range(len(points) - 1):
        t0, a0, b0 = points[i]
        t1, a1, b1 = points[i + 1]
        if t0 <= t <= t1:
            frac = (t - t0) / (t1 - t0) if t1 > t0 else 0.0
            return a0 + frac * (a1 - a0), b0 + frac * (b1 - b0)
    return points[-1][1], points[-1][2]


def ntp_error_growth(points, t, phi=PHI):
    """
    Berechnet maxerror/esterror gemaess NTP-Standard: Ausgehend vom letzten
    gemessenen Wert (aus peerstats) wachsen beide Groessen mit fester Rate
    PHI pro Sekunde, bis der naechste gemessene Wert den Verlauf zuruecksetzt.
    Kein lineares Interpolieren zwischen zwei Messpunkten, sondern ein
    Saegezahn-Verlauf ausgehend vom jeweils letzten Messpunkt.
    """
    if not points:
        return 0.0, 0.0
    if t <= points[0][0]:
        return points[0][1], points[0][2]

    # rechtesten Punkt mit t0 <= t suchen (letzter bekannter Messwert)
    idx = 0
    for i in range(len(points)):
        if points[i][0] <= t:
            idx = i
        else:
            break

    t0, a0, b0 = points[idx]
    dt = t - t0
    return a0 + phi * dt, b0 + phi * dt


def main():
    if len(sys.argv) != 5:
        print("Aufruf: python3 build_extended_log.py "
              "<offset.log> <peerstats> <loopstats> <ausgabe.log>")
        sys.exit(1)

    offset_path, peer_path, loop_path, out_path = sys.argv[1:5]

    offsets    = read_offsets(offset_path)
    peerpoints = read_peerstats(peer_path)
    looppoints = read_loopstats(loop_path)

    print(f"Offset-Zeilen:      {len(offsets)}")
    print(f"peerstats-Punkte:   {len(peerpoints)}")
    print(f"loopstats-Punkte:   {len(looppoints)}")

    with open(out_path, "w") as out:
        for sec, offset in enumerate(offsets):
            maxerror, esterror = ntp_error_growth(peerpoints, sec)
            freq,     constant = interp2(looppoints, sec)
            out.write(f"{sec}\t{offset:.9f}\t{maxerror:.9f}\t"
                      f"{esterror:.9f}\t{freq:.3f}\t{constant:.1f}\n")

    print(f"Geschrieben:        {out_path} ({len(offsets)} Zeilen)")


if __name__ == "__main__":
    main()
