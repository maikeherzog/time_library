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
static double maxerrors[MAX_OFFSETS];   
static double esterrors[MAX_OFFSETS]; 
static double freqs[MAX_OFFSETS];      
static double constants[MAX_OFFSETS];    
int offset_count = 0; 
static int initialized = 0; 
// time_t start_time = 0;
static struct timespec start_mono = {0, 0}; 

static __thread int in_hook = 0; 

static int   (*real_clock_gettime)(clockid_t, struct timespec *) = NULL;
static time_t (*real_time)(time_t *) = NULL;
static int   (*real_adjtimex)(struct timex *) = NULL;
static int   (*real_ntp_gettime)(struct ntptimeval *) = NULL;

// Zusatz:
static int (*real_gettimeofday)(struct timeval *, void *) = NULL;
static int (*real_timespec_get)(struct timespec *, int) = NULL;


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

    double c_time, c_off, c_max, c_est, c_freq, c_const;
    while (fscanf(file, "%lf %lf %lf %lf %lf %lf",
                &c_time, &c_off, &c_max, &c_est, &c_freq, &c_const) == 6) {
        if (offset_count < MAX_OFFSETS) {
            offsets[offset_count]   = c_off;
            maxerrors[offset_count] = c_max;
            esterrors[offset_count] = c_est;
            freqs[offset_count]     = c_freq;
            constants[offset_count] = c_const;
            offset_count++;
        }
    }
    fclose(file);
    fprintf(stderr, "Loaded %d offsets from %s\n", offset_count, path);

}

static void get_errors(double *maxerror, double *esterror,
                       double *freq, double *constant) {
    if (offset_count == 0) {
        *maxerror = 0.0;
        *esterror = 0.0;
        *freq     = 0.0;
        *constant = 0.0;
        return;
    }

    struct timespec ts;
    real_clock_gettime(CLOCK_MONOTONIC, &ts);
    double elapsed = (double)(ts.tv_sec - start_mono.tv_sec)
                   + (double)(ts.tv_nsec - start_mono.tv_nsec) / 1e9;

    int index = (int)elapsed;
    if (index >= offset_count - 1) {
        *maxerror = maxerrors[offset_count - 1];
        *esterror = esterrors[offset_count - 1];
        *freq     = freqs[offset_count - 1];
        *constant = constants[offset_count - 1];
        return;
    }
    if (index < 0) {
        *maxerror = maxerrors[0];
        *esterror = esterrors[0];
        *freq     = freqs[0];
        *constant = constants[0];
        return;
    }

    double frac = elapsed - index;
    *maxerror = maxerrors[index] + frac * (maxerrors[index + 1] - maxerrors[index]);
    *esterror = esterrors[index] + frac * (esterrors[index + 1] - esterrors[index]);
    *freq     = freqs[index]     + frac * (freqs[index + 1]     - freqs[index]);
    *constant = constants[index] + frac * (constants[index + 1] - constants[index]);
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

    // Zusätzlich:
    real_gettimeofday = dlsym(RTLD_NEXT, "gettimeofday");
    real_timespec_get = dlsym(RTLD_NEXT, "timespec_get");


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
    
    double sim_max, sim_est, sim_freq, sim_const;
    get_errors(&sim_max, &sim_est, &sim_freq, &sim_const);
    tx->maxerror = (long)(sim_max * 1e6);
    tx->esterror = (long)(sim_est * 1e6);
    tx->freq     = (long)sim_freq;      
    tx->constant = (long)sim_const;
    
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
    
    double sim_max, sim_est, sim_freq, sim_const;
    get_errors(&sim_max, &sim_est, &sim_freq, &sim_const);
    tx->maxerror = (long)(sim_max * 1e6);
    tx->esterror = (long)(sim_est * 1e6);
    tx->freq     = (long)sim_freq;      // schon in timex-Einheit
    tx->constant = (long)sim_const;
    
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

        // freq und constant werden hier mit abgefragt, aber nicht gesetzt:
        // die ntptimeval-Struktur besitzt diese Felder nicht (nur time,
        // maxerror, esterror, tai). Sie werden in adjtimex()/ntp_adjtime()
        // gesetzt, deren timex-Struktur die Felder enthaelt.
        double sim_max, sim_est, sim_freq, sim_const;
        get_errors(&sim_max, &sim_est, &sim_freq, &sim_const);
        (void)sim_freq;  // nicht verwendet
        (void)sim_const; // nicht verwendet
        ntv->maxerror = (long)(sim_max * 1e6);
        ntv->esterror = (long)(sim_est * 1e6);
    }

    in_hook = 0;
    return ret;
}

// Zusatz:
int gettimeofday(struct timeval *tv, void *tz) {

    if (in_hook) return real_gettimeofday(tv, tz); // stop recursion
    in_hook = 1;
    init_if_needed();

    int ret = real_gettimeofday(tv, tz);
    if (ret == 0 && tv != NULL) {
        double offset = get_offset();
        tv->tv_sec  += (long)offset;
        tv->tv_usec += (long)((offset - (long)offset) * 1e6);

        if (tv->tv_usec >= 1000000L) {
            tv->tv_sec  += 1;
            tv->tv_usec -= 1000000L;
        } else if (tv->tv_usec < 0) {
            tv->tv_sec  -= 1;
            tv->tv_usec += 1000000L;
        }
    }
    in_hook = 0;
    return ret;
}

int timespec_get(struct timespec *ts, int base) {
    if (in_hook) return real_timespec_get(ts, base);
    in_hook = 1;
    init_if_needed();

    int ret = real_timespec_get(ts, base);

    if (ret == TIME_UTC && ts != NULL) {
        double offset = get_offset();
        ts->tv_sec  += (long)offset;
        ts->tv_nsec += (long)((offset - (long)offset) * 1e9);

        if (ts->tv_nsec >= 1000000000L) {
            ts->tv_sec  += 1;
            ts->tv_nsec -= 1000000000L;
        } else if (ts->tv_nsec < 0) {
            ts->tv_sec  -= 1;
            ts->tv_nsec += 1000000000L;
        }
    }
    in_hook = 0;
    return ret;
}



