#include <stdio.h>
#include <time.h>
int main() {
    time_t t = time(NULL);
    printf("time: %ld\n", (long)t);
    printf("ctime: %s", ctime(&t));
    return 0;
}