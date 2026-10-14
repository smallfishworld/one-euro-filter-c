/* SPDX-License-Identifier: MIT */
/** @file three_dimensional.c @brief 3D variable-sample-time example. */
#include "one_euro_filter.h"
#include <stdio.h>

/** @brief Smooth 3D measured positions using actual inter-sample times. */
int main(void)
{
    const oef_vec3_t positions[] = {
        {1.0f, 2.0f, 3.0f}, {1.01f, 1.98f, 3.01f},
        {1.30f, 2.25f, 3.15f}, {1.70f, 2.50f, 3.40f}
    };
    const float dt_seconds[] = {0.02f, 0.021f, 0.019f, 0.024f};
    oef_config_t config = oef_config_default();
    oef_filter3d_t filter;
    unsigned int i;
    config.max_cutoff_hz = 8.0f;
    if (oef_init3d_ex(&filter, &config) != OEF_OK) return 1;
    for (i = 0U; i < sizeof(positions) / sizeof(positions[0]); ++i) {
        oef_vec3_t output;
        oef_status_t result = oef_update3d_ex(&filter, positions[i],
                                               dt_seconds[i], &output);
        if (result != OEF_OK) {
            fprintf(stderr, "%s: %s\n", oef_status_name(result),
                    oef_status_message(result));
            return 1;
        }
        printf("3D sample %u: (%.3f, %.3f, %.3f)\n", i,
               output.x, output.y, output.z);
    }
    return 0;
}
