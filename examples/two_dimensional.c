/* SPDX-License-Identifier: MIT */
/** @file two_dimensional.c @brief Generic 2D pointer/position filter example. */
#include "one_euro_filter.h"
#include <stdio.h>

/** @brief Smooth a sampled XY position stream with a shared vector cutoff. */
int main(void)
{
    const oef_vec2_t positions[] = {
        {100.0f, 200.0f}, {100.7f, 199.8f}, {99.9f, 200.4f},
        {101.0f, 200.8f}, {105.0f, 203.0f}, {110.0f, 206.0f}
    };
    oef_config_t config = oef_config_default();
    oef_filter2d_t filter;
    unsigned int i;
    config.sample_frequency_hz = 60.0f;
    config.beta = 0.05f; /* Example pixel coordinates; beta depends on units. */
    if (oef_init2d_ex(&filter, &config) != OEF_OK) return 1;
    for (i = 0U; i < sizeof(positions) / sizeof(positions[0]); ++i) {
        oef_vec2_t output;
        oef_status_t result = oef_step2d_ex(&filter, positions[i], &output);
        if (result != OEF_OK) {
            fprintf(stderr, "%s: %s\n", oef_status_name(result),
                    oef_status_message(result));
            return 1;
        }
        printf("2D sample %u: (%.2f, %.2f) -> (%.2f, %.2f)\n", i,
               positions[i].x, positions[i].y, output.x, output.y);
    }
    return 0;
}
