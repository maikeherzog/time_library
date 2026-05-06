# time_library

1. env.sh erstellen
    - Inhalt: export SIMULATION_LOG_PATH=/pfad/zu/simulation.log

2. env variablen setzen
    - source env.sh

3. Library erstellen
    - gcc -shared -fPIC -o libfaketime.so fake_time.c -ldl

4. Testdatei erstellen
    - gcc -o test_adjtimex test_adjtimex.c

5. Datei mit PRELOAD ausführen
    - LD_PRELOAD=./libfaketime.so ./test_ntp_gettime 
    - LD_PRELOAD=./libfaketime.so ./test_adjtimex
    - LD_PRELOAD=./libfaketime.so ./test_clock_gettime
    - LD_PRELOAD=./libfaketime.so ./test_ntp_adjtime
    - LD_PRELOAD=./libfaketime.so date


# simulation ausführen
- sudo service ntp stop
- ps aux | grep ntpd
- sudo ntpd -n -d -c ntp.conf | tee "/pfad/wo/simulationsdatei/gespeichert/werden/soll/simulation_with_statistic.log"


