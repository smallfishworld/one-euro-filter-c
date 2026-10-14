/* SPDX-License-Identifier: MIT */
/* Generic 2D example: smoothing noisy pointer / tracking coordinates. */
#include "one_euro_filter.h"
#include <stdio.h>

int main(void)
{
    const oef_vec2_t points[] = {
        {100.0f, 200.0f}, {100.7f, 199.6f},
        { 99.4f, 200.5f}, {100.5f, 199.8f},
        {102.0f, 201.0f}, {106.0f, 204.0f},
        {112.0f, 209.0f}, {115.0f, 211.0f}
    };
    const size_t count = sizeof(points) / sizeof(points[0]);
    oef_config_t config = oef_config_default();
    oef_filter2d_t filter;
    size_t i;

    config.sample_frequency_hz = 60.0f; /* fixed 60 Hz updates */
    config.min_cutoff_hz = 0.8f;
    config.beta = 0.05f; /* beta depends on coordinate units (pixels here) */
    config.derivative_cutoff_hz = 1.0f;
    config.max_cutoff_hz = 0.0f; /* optional; 0 = unlimited */

    if (!oef_init2d(&filter, &config)) {
        fputs("Failed to initialize 2D filter\n", stderr);
        return 1;
    }

    puts("index\tinput x,y\tfiltered x,y");
    for (i = 0; i < count; ++i) {
        oef_vec2_t filtered;
        if (!oef_step2d(&filter, points[i], &filtered)) {
            fputs("Failed to process 2D point\n", stderr);
            return 1;
        }
        printf("%zu\t(%.2f, %.2f)\t(%.2f, %.2f)\n", i,
               points[i].x, points[i].y, filtered.x, filtered.y);
    }
    return 0;
}
