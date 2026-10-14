/* SPDX-License-Identifier: MIT */
/** @file one_euro_clock.c @brief Built-in platform monotonic-clock backend. */
#if !defined(_WIN32) && (defined(__linux__) || defined(__APPLE__))
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include "one_euro_filter.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <time.h>
#endif

/** @brief Read monotonic seconds with no wall-clock adjustments. */
double oef_clock_monotonic(void)
{
#if defined(_WIN32)
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
        !QueryPerformanceCounter(&counter)) {
        return 0.0;
    }
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#elif defined(__linux__) || defined(__APPLE__)
    {
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
            return 0.0;
        }
        return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
    }
#else
    /* RTOS/bare-metal: inject a platform-specific oef_clock_fn callback. */
    return 0.0;
#endif
}
