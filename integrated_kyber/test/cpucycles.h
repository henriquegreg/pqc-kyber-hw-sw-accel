#ifndef CPUCYCLES_H
#define CPUCYCLES_H
#include <stdint.h>
#include <time.h>

static inline uint64_t cpucycles(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (uint64_t)(time.tv_sec * 650000000ULL + time.tv_nsec * 0.65);
}

uint64_t cpucycles_overhead(void);
#endif