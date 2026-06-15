#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main() {
    for (int i = 0; i < 100; i++) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        double t = ts.tv_sec + ts.tv_nsec * 1e-9;
        printf("%d %.9f\n", i, t);
        fflush(stdout);
        sleep(1);
    }
    return 0;
}
