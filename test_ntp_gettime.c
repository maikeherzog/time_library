#include <stdio.h>
#include <sys/timex.h>

int main() {
    struct ntptimeval ntv;

    ntp_gettime(&ntv);

    printf("Zeit: %ld\n", ntv.time.tv_sec);
    printf("Mikrosekunden: %ld\n", ntv.time.tv_usec);
    printf("Maxerror: %ld\n", ntv.maxerror);
    printf("Esterror: %ld\n", ntv.esterror);

    return 0;
}