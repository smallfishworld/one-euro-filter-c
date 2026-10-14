/* SPDX-License-Identifier: MIT */
/** @file two_dimensional.c @brief Generic XY coordinate auto-timing. */
#include "one_euro_filter.h"
#include <stdio.h>

/** @brief Demonstrate automatic timing and 2D vector filtering. */
int main(void)
{
    const oef_vec2_t points[] = {
        {100.0f, 200.0f}, {100.7f, 199.8f}, {99.9f, 200.4f},
        {101.0f, 200.8f}, {105.0f, 203.0f}, {110.0f, 206.0f}
    };
    oef_config_t config = oef_config_default();
    oef_auto_filter2d_t filter;
    unsigned int i;
    oef_status_t status;
    config.beta = 0.05f; /* Example coordinates in pixels. */
    status = oef_auto_init2d_ex(&filter, &config, NULL, NULL);
    if (status != OEF_OK) return 1;
    for (i = 0U; i < sizeof(points)/sizeof(points[0]); ++i) {
        oef_vec2_t filtered;
        status = oef_auto_update2d_ex(&filter, points[i], &filtered);
        if (status != OEF_OK) {
            fprintf(stderr, "%s: %s\n", oef_status_name(status),
                    oef_status_message(status));
            return 1;
        }
        printf("2D (%.2f, %.2f) -> (%.2f, %.2f)\n", points[i].x,
               points[i].y, filtered.x, filtered.y);
    }
    return 0;
}
