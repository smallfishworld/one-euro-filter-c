/* SPDX-License-Identifier: MIT */
/** @file one_dimensional.c @brief Scalar sensor reading smoothing example. */
#include "one_euro_filter.h"
#include <stdio.h>

/** @brief Initialize a filter and process simulated noisy samples. */
int main(void)
{
    const float readings[] = {25.0f, 25.1f, 24.9f, 25.2f, 26.1f, 26.0f};
    oef_config_t config = oef_config_default();
    oef_filter_t filter;
    unsigned int i;
    config.sample_frequency_hz = 50.0f;
    if (oef_init_ex(&filter, &config) != OEF_OK) return 1;
    for (i = 0U; i < sizeof(readings) / sizeof(readings[0]); ++i) {
        float output;
        oef_status_t result = oef_step_ex(&filter, readings[i], &output);
        if (result != OEF_OK) {
            fprintf(stderr, "%s: %s\n", oef_status_name(result),
                    oef_status_message(result));
            return 1;
        }
        printf("1D sample %u: input=%.3f output=%.3f\n", i, readings[i], output);
    }
    return 0;
}
