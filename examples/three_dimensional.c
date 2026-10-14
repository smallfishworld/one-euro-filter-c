/* SPDX-License-Identifier: MIT */
/* Generic 3D example: variable-rate spatial position measurements. */
#include "one_euro_filter.h"
#include <stdio.h>

int main(void)
{
    const oef_vec3_t points[] = {
        {1.00f, 2.00f, 3.00f}, {1.02f, 1.98f, 3.01f},
        {0.99f, 2.03f, 2.98f}, {1.20f, 2.10f, 3.08f},
        {1.55f, 2.30f, 3.25f}, {1.90f, 2.60f, 3.50f}
    };
    /* Actual elapsed time since preceding measurement, in seconds.
       The first sample is passed through, so its dt only needs to be valid. */
    const float dt_seconds[] = {
        0.020f, 0.021f, 0.018f, 0.024f, 0.019f, 0.020f
    };
    const size_t count = sizeof(points) / sizeof(points[0]);
    oef_config_t config = oef_config_default();
    oef_filter3d_t filter;
    size_t i;

    config.min_cutoff_hz = 0.8f;
    config.beta = 0.5f;
    config.derivative_cutoff_hz = 1.0f;
    config.max_cutoff_hz = 8.0f; /* optional cutoff ceiling in Hz */

    if (!oef_init3d(&filter, &config)) {
        fputs("Failed to initialize 3D filter\n", stderr);
        return 1;
    }

    puts("index\tfiltered x, y, z");
    for (i = 0; i < count; ++i) {
        oef_vec3_t filtered;
        if (!oef_update3d(&filter, points[i], dt_seconds[i], &filtered)) {
            fputs("Failed to process 3D point\n", stderr);
            return 1;
        }
        printf("%zu\t(%.3f, %.3f, %.3f)\n", i,
               filtered.x, filtered.y, filtered.z);
    }
    return 0;
}
