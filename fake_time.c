#define _GNU_SOURCE
#include <dlfcn.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/timex.h>
#include <sys/time.h>

#define MAX_OFFSETS 100000

double offsets[MAX_OFFSETS]; 
int offset_count = 0; 
static int initialized = 0; 
// time_t start_time = 0;
static struct timespec start_mono = {0, 0}; 

static __thread int in_hook = 0; 

static int   (*real_clock_gettime)(clockid_t, struct timespec *) = NULL;
static time_t (*real_time)(time_t *) = NULL;
static int   (*real_adjtimex)(struct timex *) = NULL;
static int   (*real_ntp_gettime)(struct ntptimeval *) = NULL;

/* Column 2 from offset.log*/
static void load_offsets(void){

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

    double col1, col2;
    while (offset_count < MAX_OFFSETS && fscanf(file, "%lf %lf", &col1, &col2) == 2) {
        offsets[offset_count++] = col2;
    }
    fclose(file);
    fprintf(stderr, "Loaded %d offsets from %s\n", offset_count, path);

}

static double get_offset(void){
    if (offset_count == 0) return 0.0;

    /*
    struct timespec ts;
    real_clock_gettime(CLOCK_REALTIME, &ts);
    double elapsed = (double) (ts.tv_sec - start_time) + ts.tv_nsec / 1e9;
    */
    struct timespec ts;
    real_clock_gettime(CLOCK_MONOTONIC, &ts);

    double elapsed = (double)(ts.tv_sec - start_mono.tv_sec) + (double)(ts.tv_nsec - start_mono.tv_nsec) / 1e9;

    int index = (int)elapsed;
    if (index >= offset_count -1) return offsets[offset_count -1];
    if (index < 0) return offsets[0];

    double frac = elapsed - index; // for interpolation
    return offsets[index] + frac * (offsets[index + 1] - offsets[index]);

}

static void init_if_needed(void) {
    if (initialized) return;

    initialized = 1;

    real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
    real_time = dlsym(RTLD_NEXT, "time");
    real_adjtimex = dlsym(RTLD_NEXT, "adjtimex");
    real_ntp_gettime = dlsym(RTLD_NEXT, "ntp_gettime");

    load_offsets();

    /*
    struct timespec ts;
    real_clock_gettime(CLOCK_REALTIME, &ts);
    start_time = ts.tv_sec;
    */
   real_clock_gettime(CLOCK_MONOTONIC, &start_mono);
}

/*
Hook for time()

Test with LD_PRELOAD=./libfaketime.so date
*/
time_t time(time_t *t) {
    if(in_hook) return real_time(t); // stop recursion 
    in_hook = 1;
    init_if_needed();

    struct timespec ts;
    real_clock_gettime(CLOCK_REALTIME, &ts);
    double offset = get_offset();

    time_t results = ts.tv_sec + (time_t)offset;
    if (t) *t = results;
    in_hook = 0;
    return results;

}

/*
Hook for clock_gettime()

Test with LD_PRELOAD=./libfaketime.so ./test_clock_gettime
*/
int clock_gettime(clockid_t clk_id, struct timespec *tp) {

    if (in_hook) return real_clock_gettime(clk_id, tp); // stop recursion
    in_hook = 1;
    init_if_needed();

    int ret = real_clock_gettime(clk_id, tp);
    if (ret == 0){
        double offset = get_offset();
        long offset_sec = (long)offset;
        long offset_nsec = (long)((offset - offset_sec) * 1e9);

        tp->tv_sec += offset_sec;
        tp->tv_nsec += offset_nsec;
        if (tp->tv_nsec >= 1000000000L){
            tp->tv_sec += 1;
            tp->tv_nsec -= 1000000000L;
        }else if (tp->tv_nsec < 0){
            tp->tv_sec -= 1;
            tp->tv_nsec += 1000000000L;
        }
    }
    in_hook = 0;
    return ret;

}

/*
Hook for adjtimex()

Test with LD_PRELOAD=./libfaketime.so ./test_adjtimex
*/
int adjtimex(struct timex *tx) {

    if (in_hook) return real_adjtimex(tx); // stop recursion
    in_hook = 1;
    init_if_needed();

    int ret = real_adjtimex(tx);
    double offset = get_offset();

    if (tx->status & STA_NANO)
        tx->offset = (long)(offset * 1e9);   // Nanosekunden
    else
        tx->offset = (long)(offset * 1e6);   // Mikrosekunden
    in_hook = 0;
    return ret;

}

int ntp_adjtime(struct timex *tx) {

    if (in_hook) return real_adjtimex(tx); // stop recursion
    in_hook = 1;
    init_if_needed();

    int ret = real_adjtimex(tx);
    double offset = get_offset();

    if (tx->status & STA_NANO)
        tx->offset = (long)(offset * 1e9);   // Nanosekunden
    else
        tx->offset = (long)(offset * 1e6);   // Mikrosekunden
    in_hook = 0;
    return ret;
}

/*
Hook for ntp_gettime()

Test with LD_PRELOAD=./libfaketime.so ./test_ntp_gettime
*/
int ntp_gettime(struct ntptimeval *ntv) {

    if (in_hook) return real_ntp_gettime(ntv); // stop recursion
    in_hook = 1;
    init_if_needed();

    int ret = real_ntp_gettime(ntv);
    fprintf(stderr, "VORHER:  ret=%d tv_sec=%ld tv_usec=%ld\n",
            ret, ntv->time.tv_sec, ntv->time.tv_usec);

    if (ret >= 0) {
        double offset = get_offset();

        // STA_NANO-Status separat ueber adjtimex abfragen,
        // da ntptimeval selbst kein status-Feld besitzt
        struct timex tx;
        tx.modes = 0;                 
        real_adjtimex(&tx);
        long scale = (tx.status & STA_NANO) ? 1000000000L : 1000000L;

        ntv->time.tv_sec  += (long)offset;
        ntv->time.tv_usec += (long)((offset - (long)offset) * scale);

        if (ntv->time.tv_usec >= scale) {
            ntv->time.tv_sec  += 1;
            ntv->time.tv_usec -= scale;
        } else if (ntv->time.tv_usec < 0) {
            ntv->time.tv_sec  -= 1;
            ntv->time.tv_usec += scale;
        }
    }

    in_hook = 0;
    return ret;
}



