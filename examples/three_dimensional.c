/* SPDX-License-Identifier: MIT */
/** @file three_dimensional.c @brief Portable callback-based 3D tracking. */
#include "one_euro_filter.h"
#include "example_clock.h"
#include <stdio.h>

/** @brief Automatically derive intervals from a custom platform clock. */
int main(void)
{
    const oef_vec3_t points[] = {
        {1.0f, 2.0f, 3.0f}, {1.01f, 1.98f, 3.01f},
        {1.30f, 2.25f, 3.15f}, {1.70f, 2.50f, 3.40f}
    };
    oef_config_t config = oef_config_default();
    oef_auto_filter3d_t filter;
    oef_example_clock_t timer = {0.0, 0.020};
    unsigned int i;
    oef_status_t status = oef_auto_init3d_ex(&filter, &config,
                                              oef_example_clock, &timer);
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
