/* SPDX-License-Identifier: MIT */
/* Generic 1D example: smoothing a noisy scalar sensor stream. */
#include "one_euro_filter.h"
#include <stdio.h>

int main(void)
{
    const float readings[] = {
        25.00f, 25.12f, 24.91f, 25.08f, 24.96f,
        25.03f, 25.48f, 25.91f, 26.10f, 26.04f
    };
    const size_t count = sizeof(readings) / sizeof(readings[0]);
    oef_config_t config = oef_config_default();
    oef_filter_t filter;
    size_t i;

    /* The sensor produces one sample every 20 ms (50 Hz). */
    config.sample_frequency_hz = 50.0f;
    config.min_cutoff_hz = 0.8f;
    config.beta = 0.5f;
    config.derivative_cutoff_hz = 1.0f;

    if (!oef_init(&filter, &config)) {
        fputs("Failed to initialize 1D filter\n", stderr);
        return 1;
    }

    puts("index\tmeasurement\tfiltered");
    for (i = 0; i < count; ++i) {
        float filtered;
        if (!oef_step(&filter, readings[i], &filtered)) {
            fputs("Failed to process sample\n", stderr);
            return 1;
        }
        printf("%zu\t%.3f\t\t%.3f\n", i, readings[i], filtered);
    }
    return 0;
}
