/* SPDX-License-Identifier: MIT */
/** @file one_dimensional.c @brief Automatic-clock scalar smoothing example. */
#include "one_euro_filter.h"
#include "example_clock.h"
#include <stdio.h>

/** @brief Filter independent 1D measurements without sample-rate configuration. */
int main(void)
{
    const float measurements[] = {25.00f, 25.10f, 24.90f, 25.20f, 26.10f};
    oef_config_t config = oef_config_default();
    oef_auto_filter_t filter;
    unsigned int i;
    oef_example_clock_t demo_clock = {0.0, 0.020};
    oef_status_t status = oef_auto_init_ex(&filter, &config,
                                             oef_example_clock, &demo_clock);
    if (status != OEF_OK) return 1;
    for (i = 0U; i < sizeof(measurements)/sizeof(measurements[0]); ++i) {
        float filtered;
        status = oef_auto_update_ex(&filter, measurements[i], &filtered);
        if (status != OEF_OK) {
            fprintf(stderr, "%s: %s\n", oef_status_name(status),
                    oef_status_message(status));
            return 1;
        }
        printf("1D input=%7.3f output=%7.3f\n", measurements[i], filtered);
    }
    return 0;
}
