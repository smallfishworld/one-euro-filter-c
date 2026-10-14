/* SPDX-License-Identifier: MIT */
/** @file example_clock.h @brief Deterministic mock clock for fast demos. */
#ifndef OEF_EXAMPLE_CLOCK_H
#define OEF_EXAMPLE_CLOCK_H
#include "one_euro_filter.h"
#include <stddef.h>
/** @brief Mock monotonic clock; useful when demo samples arrive immediately. */
typedef struct {
    double seconds;       /**< Last generated timestamp in seconds. */
    double step_seconds;  /**< Virtual time increment per sample. */
} oef_example_clock_t;
/** @brief Clock callback; returns a synthetic increasing timestamp. */
static oef_status_t oef_example_clock(void *context, double *seconds)
{
    oef_example_clock_t *timer = (oef_example_clock_t *)context;
    if (timer == NULL || seconds == NULL || timer->step_seconds <= 0.0)
        return OEF_E_CLOCK;
    timer->seconds += timer->step_seconds;
    *seconds = timer->seconds;
    return OEF_OK;
}
#endif /* OEF_EXAMPLE_CLOCK_H */
