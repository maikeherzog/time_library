#define _GNU_SOURCE
#include <dlfcn.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/timex.h>
#include <sys/time.h>

#define MAX_OFFSETS 10000

double offsets[MAX_OFFSETS]; //gespeicherte Offsets
int offset_count = 0; // Anzahl der gespeicherten Offsets
static int initialized = 0; 
time_t start_time = 0; //Referenzzeitpunkt

static __thread int in_init = 0; // Flag, um Rekursion zu verhindern
static __thread int in_time = 0; // Flag, um Rekursion in time() zu verhindern

static time_t (*real_time_func)(time_t *) = NULL;


void load_offsets(){

    const char *path = getenv("SIMULATION_LOG_PATH");
    if (!path) {
        fprintf(stderr, "SIMULATION_LOG_PATH environment variable not set\n");
        return;
    }
    
    FILE *file = fopen(path, "r");

    if (!file) {
        fprintf(stderr, "Could not open offsets file\n");
        return;
    }

    char line[512];
    
    while (fgets(line, sizeof(line), file)) {

        if (strstr(line, "combine offset")){
            char *ptr = strstr(line, "offset");
            if (ptr) {
                ptr += strlen("offset");
                
                // to skip leading spaces
                while (*ptr == ' ') ptr++;

                double offset = strtod(ptr, NULL);

                if (offset_count < MAX_OFFSETS) {
                    offsets[offset_count++] = offset;
                    //printf("Loaded offset: %f\n", offset);
                } else {
                    fprintf(stderr, "Max offsets reached\n");
                    break;
                }
            }
        }
    }
    fclose(file);

    printf("Loaded %d offsets\n", offset_count);
}

double get_offset(){
    if (offset_count == 0) return 0.0;

    time_t now = time(NULL);
    printf("Current time: %ld\n", now);
    printf("Start time: %ld\n", start_time);
    //int index = (now - start_time) % offset_count; 
    
    // Für Interpolation 
    double elapsed = difftime(now, start_time);
    printf("Elapsed time: %f seconds\n", elapsed);
    int index = (int)elapsed % offset_count;
    printf("Using offset index: %d\n", index);
    int next_index = (index + 1) % offset_count;
    printf("Next offset index: %d\n", next_index);

    double frac = elapsed - (int)elapsed; // Bruchteil für Interpolation

    double o1 = offsets[index];
    double o2 = offsets[next_index];

    return o1 + frac * (o2 - o1); // Lineare Interpolation

    //return offsets[index];
}

void init_if_needed() {
    if (initialized || in_init) return;

    in_init = 1;

    load_offsets();

    if (!real_time_func) real_time_func = dlsym(RTLD_NEXT, "time");
    start_time = real_time_func(NULL);
    initialized = 1;

    in_init = 0;
}

/*
Hook für time()

Test mit LD_PRELOAD=./libfaketime.so date
*/
time_t time(time_t *t) {
    static int logged = 0;
    if(in_time) return real_time_func(t); // Rekursion verhindern
    in_time = 1;

    if (!logged) {
        printf("Fake time called\n");
        logged = 1;
    }
    //fprintf(stderr, "Fake time called\n");
    static time_t (*real_time)(time_t *) = NULL;

    if (!real_time) {
        real_time = dlsym(RTLD_NEXT, "time");
    }

    init_if_needed();

    time_t real = real_time(NULL);

    double offset = get_offset();

    time_t fake = real + (time_t)offset;

    if (t) *t = fake;
    return fake;
}

/*
Hook für clock_gettime()

Test mit LD_PRELOAD=./libfaketime.so ./test_clock_gettime
*/
int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    printf("Fake clock_gettime called\n");

    static int (*real_clock_gettime)(clockid_t, struct timespec*) = NULL;
    static time_t (*real_time_func)(time_t *) = NULL;

    if (!real_clock_gettime)
        real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");

    int ret = real_clock_gettime(clk_id, tp);

    init_if_needed();

    double offset = get_offset();
    //tp->tv_sec += (time_t)offset();
    // um keine Nachkommastellen zu verlieren
    tp->tv_sec += (long)((offset - (time_t)offset) * 1e9);
    //tp->tv_sec += 3600; // hier ersetze ich das dann mit dem Offset

    return ret;
}

/*
Hook für ntp_gettime()

Test mit LD_PRELOAD=./libfaketime.so ./test_ntp_gettime
*/
int ntp_gettime(struct ntptimeval *ntv) {
    printf("Fake ntp_gettime called\n");
    static int (*real_ntp_gettime)(struct ntptimeval *) = NULL;

    if (!real_ntp_gettime)
        real_ntp_gettime = dlsym(RTLD_NEXT, "ntp_gettime");

    int ret = real_ntp_gettime(ntv);

    init_if_needed();

    // Offset berechnen
    double offset = get_offset();

    // Zeit manipulieren
    ntv->time.tv_sec += (time_t)offset;
    ntv->time.tv_sec += (suseconds_t)((offset - (time_t)offset) * 1e9);

    // Fehlerwerte setzen
    ntv->maxerror = 500000; // 50ms
    ntv->esterror = 1000000;  // 10ms

    return ret;
}

/*
Hook für adjtimex()

Test mit LD_PRELOAD=./libfaketime.so ./test_adjtimex
*/
int adjtimex(struct timex *tx) {
    printf("Fake adjtimex called\n");

    // Struktur zurücksetzen
    memset(tx, 0, sizeof(struct timex)); 
    
    // Initialisierung 
    init_if_needed();

    double offset = get_offset();

    tx->modes = ADJ_OFFSET;
    tx->offset = (long)(offset * 1e6);
    tx->freq = 0;
    tx->status = 0;

    return 0;
}

int ntp_adjtime(struct timex *tx) {
    printf("Fake ntp_adjtime called\n");

    // Struktur zurücksetzen
    memset(tx, 0, sizeof(struct timex)); 

    // Initialisierung 
    init_if_needed();

    double offset = get_offset();

    //tx->modes = ADJ_OFFSET;
    tx->offset = (long)(offset * 1e6);
    /*
    tx->freq = 0;
    tx->status = 0;
    */
    
    return 0;
}