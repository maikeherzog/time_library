#include <stdio.h>
#include <sys/timex.h>

int main() {
    struct timex tx;

    int ret = adjtimex(&tx);

    printf("return: %d\n", ret);
    printf("offset: %ld\n", tx.offset);
    printf("freq: %ld\n", tx.freq);
    printf("maxerror: %ld\n", tx.maxerror);
    printf("esterror: %ld\n", tx.esterror);
    printf("status: %d\n", tx.status);
    printf("constant: %ld\n", tx.constant);

    return 0;
}