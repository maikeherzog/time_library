#include <stdio.h>
#include <time.h>

int main() {
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    printf("tv_sec: %ld\n", ts.tv_sec);
    printf("tv_nsec: %ld\n", ts.tv_nsec);

    return 0;
}