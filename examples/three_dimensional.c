/* SPDX-License-Identifier: MIT */
/** @file three_dimensional.c
 *  @brief Realistic 3D integration: smooth tracked positions on an RTOS,
 *  where a custom tick source replaces the OS monotonic clock.
 *
 *  The pattern to copy: obtain a monotonic seconds value from your platform
 *  (hardware timer, RTOS tick, performance counter), pass the reading
 *  function to oef_auto_init3d(), then feed every tracked position through
 *  oef_auto_update3d().
 */
#include "one_euro_filter.h"
#include <math.h>
#include <stdio.h>

/** @brief Hardware timer state, as an RTOS driver would expose it. */
typedef struct {
    long tick;               /**< Monotonically increasing tick count. */
    double ticks_per_second; /**< Timer frequency in Hz. */
} hw_timer_t;

/** @brief The one custom function an RTOS integration needs to provide. */
static double hw_timer_seconds(void *context)
{
    const hw_timer_t *timer = (const hw_timer_t *)context;
    /* Real code: read the hardware counter and handle rollover. */
    return (double)timer->tick / timer->ticks_per_second;
}

/** @brief Stand-in for a 3D position tracker: true path plus noise. */
static oef_vec3_t read_tracker(int tick)
{
    const oef_vec3_t true_position = {1.0f + 0.8f * sinf(tick * 0.1f),
                                      2.0f + 0.6f * cosf(tick * 0.1f),
                                      3.0f + 0.4f * sinf(tick * 0.08f)};
    const oef_vec3_t noise = {0.02f * sinf(tick * 3.7f),
                              0.02f * cosf(tick * 4.9f),
                              0.02f * sinf(tick * 5.3f)};
    oef_vec3_t raw = {true_position.x + noise.x,
                      true_position.y + noise.y,
                      true_position.z + noise.z};
    return raw;
}

/** @brief Simulate a 50 Hz tracker feeding the filter each sample. */
int main(void)
{
    hw_timer_t timer = {0, 50.0};
    oef_auto_filter3d_t filter;
    int tick;
    /* Inject the platform clock; config NULL keeps the defaults. */
    if (oef_auto_init3d(&filter, NULL, hw_timer_seconds, &timer) != OEF_OK) {
        return 1;
    }

    for (tick = 0; tick < 60; ++tick) {
        oef_vec3_t raw = read_tracker(tick);
        oef_vec3_t smoothed;
        ++timer.tick; /* Real code: the hardware timer increments itself. */
        if (oef_auto_update3d(&filter, raw, &smoothed) != OEF_OK) continue;
        printf("raw=(%.3f, %.3f, %.3f)  smooth=(%.3f, %.3f, %.3f)\n",
               raw.x, raw.y, raw.z, smoothed.x, smoothed.y, smoothed.z);
    }
    return 0;
}
