#include <stdio.h>
#include <time.h>

int main() {
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    printf("Sekunden: %ld\n", ts.tv_sec);
    printf("Nanosekunden: %ld\n", ts.tv_nsec);

    return 0;
}