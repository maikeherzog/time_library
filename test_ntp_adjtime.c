#include <stdio.h>
#include <sys/timex.h>

int main() {
    struct timex tx;

    ntp_adjtime(&tx);

    printf("Offset: %ld\n", tx.offset);
    printf("Status: %d\n", tx.status);
    printf("Frequency: %ld\n", tx.freq);
    printf("Maxerror: %ld\n", tx.maxerror);
    printf("Esterror: %ld\n", tx.esterror);
    printf("Tolerance: %ld\n", tx.tolerance);

    return 0;
}