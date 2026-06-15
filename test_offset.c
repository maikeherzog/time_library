#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(){
    struct timespec ts_real, ts_fake;

    for (int i = 0; i < 100; i++){
        clock_gettime(CLOCK_REALTIME, &ts_fake);

        double fake_time = ts_fake.tv_sec + ts_fake.tv_nsec / 1e9;

        printf("Fake time: %f\n", fake_time);
        sleep(1);
    }
    return 0;
}