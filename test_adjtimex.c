#include <stdio.h>
#include <sys/timex.h>

int main() {
    struct timex tx;

    int ret = adjtimex(&tx);

    printf("Return: %d\n", ret);
    printf("Offset (us): %ld\n", tx.offset);
    printf("Frequency: %ld\n", tx.freq);
    printf("Status: %d\n", tx.status);
    printf("Maxerror: %ld\n", tx.maxerror);
    printf("Esterror: %ld\n", tx.esterror);
    printf("Tolerance: %ld\n", tx.tolerance);

    return 0;
}