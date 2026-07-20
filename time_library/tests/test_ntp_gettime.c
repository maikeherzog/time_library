#include <stdio.h>
#include <sys/timex.h>

int main() {
    struct ntptimeval ntv;

    ntp_gettime(&ntv);

    printf("tv_sec: %ld\n", ntv.time.tv_sec);
    printf("tv_usec: %ld\n", ntv.time.tv_usec);
    printf("maxerror: %ld\n", ntv.maxerror);
    printf("esterror: %ld\n", ntv.esterror);
    printf("tai: %ld\n", ntv.tai);

    return 0;
}