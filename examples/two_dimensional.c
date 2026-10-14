/* SPDX-License-Identifier: MIT */
/** @file two_dimensional.c
 *  @brief Realistic 2D integration: smooth jittery pointer/touch coordinates.
 *
 *  The pattern to copy: create the filter next to your input device, feed
 *  every pointer event through oef_auto_update2d(), render the filtered
 *  position instead of the raw one. The sleep stands in for the natural
 *  ~60 Hz event spacing; in a real event loop the samples already arrive
 *  spaced in time, so just drop it.
 */
#define _POSIX_C_SOURCE 200809L /* nanosleep */
#include "one_euro_filter.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

/** @brief Stand-in for a tracking camera: true position plus hand jitter. */
static oef_vec2_t read_pointer(int tick)
{
    const oef_vec2_t true_position = {400.0f + 300.0f * sinf(tick * 0.02f),
                                      300.0f + 200.0f * cosf(tick * 0.02f)};
    const oef_vec2_t jitter = {2.0f * sinf(tick * 5.1f),
                               2.0f * cosf(tick * 4.3f)};
    oef_vec2_t raw = {true_position.x + jitter.x,
                      true_position.y + jitter.y};
    return raw;
}

/** @brief Simulate a 60 Hz pointer event loop with jittery coordinates. */
int main(void)
{
    oef_config_t config = oef_config_default();
    oef_auto_filter2d_t filter;
    int tick;
    /* Pixels move fast: lower beta than the default suits pointer data. */
    config.beta = 0.05f;
    if (oef_auto_init2d(&filter, &config, NULL, NULL) != OEF_OK) return 1;

    for (tick = 0; tick < 60; ++tick) {
        oef_vec2_t raw = read_pointer(tick);
        oef_vec2_t smoothed;
        struct timespec gap = {0, 16 * 1000000L}; /* 16 ms = ~60 Hz. */
        nanosleep(&gap, NULL); /* Real code: your event loop paces itself. */
        if (oef_auto_update2d(&filter, raw, &smoothed) != OEF_OK) continue;
        printf("raw=(%7.2f, %7.2f)  smooth=(%7.2f, %7.2f)\n",
               raw.x, raw.y, smoothed.x, smoothed.y);
    }
    return 0;
}
