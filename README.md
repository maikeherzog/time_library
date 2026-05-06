# time_library

1. env variablen setzen
    - source env.sh

2. Library erstellen
    - gcc -shared -fPIC -o libfaketime.so fake_time.c -ldl

3. Testdatei erstellen
    - gcc -o test_adjtimex test_adjtimex.c

4. Datei mit PRELOAD ausführen
    - LD_PRELOAD=./libfaketime.so ./test_ntp_gettime 
    - LD_PRELOAD=./libfaketime.so ./test_adjtimex
    - LD_PRELOAD=./libfaketime.so ./test_clock_gettime
    - LD_PRELOAD=./libfaketime.so ./test_ntp_adjtime
    - LD_PRELOAD=./libfaketime.so date

