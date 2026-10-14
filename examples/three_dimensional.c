/* SPDX-License-Identifier: MIT */
/** @file three_dimensional.c @brief Portable callback-based 3D tracking. */
#include "one_euro_filter.h"
#include <stdio.h>

/** @brief Mock monotonic clock; replace with RTOS tick counter on hardware. */
typedef struct {
    double seconds; /**< Last clock reading in seconds. */
} example_clock_t;

/** @brief Platform clock callback that requires no system clock API. */
static oef_status_t example_clock(void *context, double *seconds)
{
    example_clock_t *timer = (example_clock_t *)context;
    if (timer == NULL || seconds == NULL) return OEF_E_NULL;
    timer->seconds += 0.02; /* Simulate 50 Hz readings. */
    *seconds = timer->seconds;
    return OEF_OK;
}

/** @brief Automatically derive intervals from a custom platform clock. */
int main(void)
{
    const oef_vec3_t points[] = {
        {1.0f, 2.0f, 3.0f}, {1.01f, 1.98f, 3.01f},
        {1.30f, 2.25f, 3.15f}, {1.70f, 2.50f, 3.40f}
    };
    oef_config_t config = oef_config_default();
    oef_auto_filter3d_t filter;
    example_clock_t timer = {0.0};
    unsigned int i;
    oef_status_t status = oef_auto_init3d_ex(&filter, &config,
                                              example_clock, &timer);
    if (status != OEF_OK) return 1;
    for (i = 0U; i < sizeof(points)/sizeof(points[0]); ++i) {
        oef_vec3_t filtered;
        status = oef_auto_update3d_ex(&filter, points[i], &filtered);
        if (status != OEF_OK) {
            fprintf(stderr, "%s: %s\n", oef_status_name(status),
                    oef_status_message(status));
            return 1;
        }
        printf("3D (%.3f, %.3f, %.3f)\n", filtered.x, filtered.y, filtered.z);
    }
    return 0;
}
