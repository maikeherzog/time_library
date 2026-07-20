# time_library

Shared Library, die simulierte NTP-Bedingungen mittels `LD_PRELOAD` in Testprogramme einspielt.

## 0. Voraussetzungen

clknetsim muss separat installiert werden:
```bash
git clone https://gitlab.com/chrony/clknetsim
cd clknetsim
make
```
Zusätzlich werden `chronyd` und `ntpd` (z. B. Version 4.2.8p18) benötigt.

Der in `client_ntp.conf` über `statsdir` angegebene Ordner für die Statistikdateien muss vorher angelegt werden:
```bash
mkdir -p ntpstats
```

## 1. Simulation mit clknetsim ausführen

Configs (in `clknetsim/`):
| Datei | Zweck |
|---|---|
| `sim.conf` | Basis aus clknetsim Doku, 100 ms Anfangsversatz, zufällige Frequenzabweichung |
| `sim_ohne_offset_node2.conf` | wie oben, ohne Anfangsversatz |
| `sim_fester_wert_ohne_offset.conf` | ohne Anfangsversatz, feste Frequenzabweichung (`node2_freq = 5e-6`) |
| `sim_zittern.conf` | starkes, unrealistisches Frequenzrauschen (`node2_freq = (* 1e-3 (normal))`) |

Start (Beispiel mit `sim.conf`, `-l` = Laufzeit in Sekunden):
```bash
./clknetsim -v 3 -R 1 -o offset.log -f freq.log -l 10000 sim.conf 2 &
sleep 1
LD_PRELOAD=./clknetsim.so CLKNETSIM_NODE=1 chronyd -d -f server.conf &
LD_PRELOAD=./clknetsim.so CLKNETSIM_NODE=2 /pfad/zu/ntpd -n -c client_ntp.conf &
wait
```
Nach jedem Lauf `ntpstats/peerstats` und `ntpstats/loopstats` umbenennen, bevor der nächste Lauf sie überschreibt.

## 2. Erweiterte Logdatei erzeugen

```bash
python3 clknetsim/build_extended_log.py offset.log peerstats loopstats offset_extended.log
```
Spalten: Zeit, Offset, `maxerror`, `esterror`, `freq`, `constant`. `maxerror`/`esterror` wachsen mit fester Rate Φ = 15 µs/s ab dem letzten Messwert (NTP-Standard), `freq`/`constant` werden linear interpoliert.

## 3. Library bauen und nutzen

```bash
gcc -shared -fPIC -o libfaketime.so fake_time.c -ldl
export SIMULATION_LOG_PATH=/pfad/zu/offset_extended.log
LD_PRELOAD=./libfaketime.so ./testprogramm
```

## 4. Testprogramme
Die `test_*`-Programme dienen dem Nachweis, dass die überschriebenen libc-Funktionen korrekt funktionieren. Jedes Programm ruft eine der überschriebenen Funktionen auf und gibt deren Rückgabewert aus. Mit vorgeladener Library (`LD_PRELOAD=./libfaketime.so`) zeigt sich, dass die zurückgegebene Zeit dem simulierten Offset-Verlauf folgt statt der echten Systemzeit.


```bash
gcc -o test_adjtimex test_adjtimex.c   # analog für die anderen test_*.c
```
| Programm | sudo? |
|---|---|---|
| `test_time`, `test_clock_gettime`, `test_gettimeofday`, `test_timespec_get`  | nein |
| `test_adjtimex`, `test_ntp_adjtime` | ja |
| `test_ntp_gettime` | nein |
| `test_verlauf [dauer] [schritt]` | nein |
| `test_zeitvergleich` | nein |

## 5. Diagramme

| Skript | Erzeugt | Benötigte Dateien |
|---|---|---|
| `clknetsim/plot_zittern.py` | Abbildung 3 | Offset-Log aus `sim_zittern.conf`-Lauf (mit 100 ms Versatz) |
| `clknetsim/plot_vergleich_clients.py` | Abbildung 2 | Offset-Logs aus `sim.conf`-Lauf, je einmal mit chronyd und ntpd als Client |
| `clknetsim/plot_vergleich_drift_200000.py` | Abbildung 1 | Offset-Logs aus 200 000 s Läufen mit `sim_ohne_offset_node2.conf` und `sim_fester_wert_ohne_offset.conf` |
| `plot_verlauf_vergleich.py` | Abbildung 6 | `offset_extended.log` (Soll) + Ausgabe von `test_verlauf` (Ist) |
| `plot_zeit_gross.py` | Abbildung 5 | Ausgabe von `test_zeitvergleich` (mit künstlich großem Demo-Offset) |



