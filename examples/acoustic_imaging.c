/* SPDX-License-Identifier: MIT */
/* Smooth the sub-grid peak before rendering a 40x32 sound map. */
#include "one_euro_filter.h"
#include <stdio.h>

int main(void)
{
    oef_filter2d_t tracker;
    oef_config_t cfg = oef_config_default();
    const oef_vec2_t measured[] = {
        {19.45f, 15.52f}, {19.51f, 15.48f},
        {19.44f, 15.54f}, {19.50f, 15.49f},
        {20.10f, 15.55f}, {20.50f, 15.60f}
    };
    unsigned int i;

    cfg.min_cutoff_hz = 0.8f;
    cfg.beta = 0.5f;
    cfg.derivative_cutoff_hz = 1.0f;
    cfg.sample_frequency_hz = 25.0f;
    cfg.max_cutoff_hz = 0.0f;

    if (!oef_init2d(&tracker, &cfg)) {
        return 1;
    }

    for (i = 0; i < sizeof(measured) / sizeof(measured[0]); ++i) {
        oef_vec2_t stable;
        if (!oef_step2d(&tracker, measured[i], &stable)) {
            return 1;
        }
        /* Use the same stable center for the heatmap and top marker. */
        printf("frame=%u raw=(%.3f, %.3f) smooth=(%.3f, %.3f) "
               "screen=(%.1f, %.1f)\n", i,
               measured[i].x, measured[i].y, stable.x, stable.y,
               stable.x * (640.0f / 40.0f),
               stable.y * (480.0f / 32.0f));
    }
    return 0;
}
