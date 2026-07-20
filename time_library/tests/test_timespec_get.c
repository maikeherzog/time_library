#include <stdio.h>
#include <time.h>
int main() {
    struct timespec ts;
    int ret = timespec_get(&ts, TIME_UTC);
    printf("ret:     %d\n", ret);
    printf("tv_sec:  %ld\n", ts.tv_sec);
    printf("tv_nsec: %ld\n", ts.tv_nsec);
    return 0;
}