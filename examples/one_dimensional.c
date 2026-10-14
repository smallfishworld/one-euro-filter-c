/* SPDX-License-Identifier: MIT */
/** @file one_dimensional.c
 *  @brief Realistic 1D integration: smooth a noisy temperature-like sensor.
 *
 *  The pattern to copy into your own code: create one filter, feed every
 *  raw sample through oef_auto_update(), use the filtered result.
 *  The sleep stands in for waiting on the real sensor (20 Hz); remove it
 *  and the NULL clock works unchanged in your own acquisition loop.
 */
#define _POSIX_C_SOURCE 200809L /* nanosleep */
#include "one_euro_filter.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

/** @brief Stand-in for a real sensor read: true value plus measurement noise. */
static float read_sensor(int tick)
{
    const float true_value = 25.0f + 0.5f * sinf(tick * 0.05f);
    const float noise = 0.3f * sinf(tick * 7.3f) * cosf(tick * 2.1f);
    return true_value + noise;
}

/** @brief Simulate a 20 Hz acquisition loop with a noisy 1D sensor. */
int main(void)
{
    oef_auto_filter_t filter;
    int tick;
    /* NULL config = sensible defaults; NULL clock = OS monotonic clock. */
    if (oef_auto_init(&filter, NULL, NULL, NULL) != OEF_OK) return 1;

    for (tick = 0; tick < 60; ++tick) {
        float raw = read_sensor(tick);
        float filtered;
        struct timespec gap = {0, 50 * 1000000L}; /* 50 ms = 20 Hz. */
        nanosleep(&gap, NULL); /* Real code: block on the sensor instead. */
        if (oef_auto_update(&filter, raw, &filtered) != OEF_OK) continue;
        printf("raw=%7.3f  filtered=%7.3f\n", raw, filtered);
    }
    return 0;
}
