#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
 
/* manipulierte Zeit: ueber libc -> geht durch die vorgeladene Library */
static double faked_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
 
/* echte Zeit: ueber rohen Syscall -> umgeht die Library */
static double real_time_raw(void)
{
    struct timespec ts;
    syscall(SYS_clock_gettime, CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
 
int main(int argc, char **argv)
{
    int dauer   = (argc > 1) ? atoi(argv[1]) : 600;
    int schritt = (argc > 2) ? atoi(argv[2]) : 1;
 
    if (dauer   <= 0) dauer   = 600;
    if (schritt <= 0) schritt = 1;
 
    double start_real = real_time_raw();
 
    fprintf(stderr, "Messung ueber %d s, Abstand %d s\n", dauer, schritt);
 
    for (int t = 0; t <= dauer; t += schritt) {
 
        double real  = real_time_raw();   /* echte Zeit (ohne Library)      */
        double faked = faked_time();      /* manipulierte Zeit (mit Library)*/
 
        double elapsed    = real  - start_real;   /* verstrichene Realzeit    */
        double real_rel   = real  - start_real;   /* echte Zeit, relativ      */
        double faked_rel  = faked - start_real;   /* manipulierte Zeit, relativ*/
 
        printf("%.1f\t%.6f\t%.6f\n", elapsed, real_rel, faked_rel);
        fflush(stdout);
 
        if (t + schritt <= dauer)
            sleep(schritt);
    }
 
    return 0;
}