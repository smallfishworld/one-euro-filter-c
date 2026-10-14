/* SPDX-License-Identifier: MIT */
/** @file one_euro_clock.c @brief Optional platform monotonic-clock backend. */
#if !defined(_WIN32) && (defined(__linux__) || defined(__APPLE__))
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include "one_euro_filter.h"
#include <math.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <time.h>
#endif

/** @brief Read monotonic seconds with no wall-clock adjustments. */
oef_status_t oef_clock_monotonic(void *context, double *seconds)
{
    double value;
    (void)context;
    if (seconds == NULL) return OEF_E_NULL;
#if defined(_WIN32)
    {
        LARGE_INTEGER frequency;
        LARGE_INTEGER counter;
        if (!QueryPerformanceFrequency(&frequency) ||
            !QueryPerformanceCounter(&counter) || frequency.QuadPart <= 0) {
            return OEF_E_CLOCK;
        }
        value = (double)counter.QuadPart / (double)frequency.QuadPart;
    }
#elif defined(__linux__) || defined(__APPLE__)
    {
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return OEF_E_CLOCK;
        value = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
    }
#else
    /* RTOS/bare-metal: inject a platform-specific oef_clock_fn callback. */
    return OEF_E_CLOCK;
#endif
    if (!isfinite(value)) return OEF_E_CLOCK;
    *seconds = value;
    return OEF_OK;
}
