#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static inline double now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main() {
    printf("strcpy 1 MB × %d times: %.2f ms\n", reps, (t1 - t0) / 1e6);
    return 0;
}
