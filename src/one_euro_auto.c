/* SPDX-License-Identifier: MIT */
/** @file one_euro_auto.c @brief Automatic timing adapters for scalar/vectors. */
#include "one_euro_filter.h"
#include <float.h>
#include <math.h>
#include <stddef.h>

/** @brief Check sample time and form dt, without advancing timestamp. */
static oef_status_t oef_auto_dt(bool has_timestamp, double previous,
                                double now, float *dt)
{
    double interval;
    if (!isfinite(now)) return OEF_E_CLOCK;
    if (!has_timestamp) {
        *dt = 1.0f; /* First sample passes through unchanged in the core. */
        return OEF_OK;
    }
    if (!isfinite(previous)) return OEF_E_STATE;
    interval = now - previous;
    if (!isfinite(interval) || interval <= 0.0) return OEF_E_TIME;
    if (interval > (double)FLT_MAX ||
        interval < (double)FLT_MIN * (double)FLT_EPSILON) return OEF_E_RANGE;
    *dt = (float)interval;
    if (!isfinite(*dt) || *dt <= 0.0f) return OEF_E_RANGE;
    return OEF_OK;
}

/** @brief Read clock callback, mapping any unsuccessful read to CLOCK error. */
static oef_status_t oef_auto_get_time(oef_clock_fn clock, void *context,
                                       double *seconds)
{
    oef_status_t st;
    /* Unwritten callback output must be detected, not read uninitialized. */
    *seconds = NAN;
    st = (clock != NULL ? clock : oef_clock_monotonic)(context, seconds);
    if (st != OEF_OK) return OEF_E_CLOCK;
    return isfinite(*seconds) ? OEF_OK : OEF_E_CLOCK;
}

/** @brief Transactional scalar adapter initialization. */
oef_status_t oef_auto_init_ex(oef_auto_filter_t *filter,
                              const oef_config_t *config,
                              oef_clock_fn clock, void *context)
{
    oef_auto_filter_t next = {0};
    oef_status_t st;
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    st = oef_init_ex(&next.filter, config);
    if (st != OEF_OK) return st;
    next.clock = clock;
    next.context = context;
    *filter = next;
    return OEF_OK;
}

/** @brief Transactional 2D adapter initialization. */
oef_status_t oef_auto_init2d_ex(oef_auto_filter2d_t *filter,
                                const oef_config_t *config,
                                oef_clock_fn clock, void *context)
{
    oef_auto_filter2d_t next = {0};
    oef_status_t st;
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    st = oef_init2d_ex(&next.filter, config);
    if (st != OEF_OK) return st;
    next.clock = clock;
    next.context = context;
    *filter = next;
    return OEF_OK;
}

/** @brief Transactional 3D adapter initialization. */
oef_status_t oef_auto_init3d_ex(oef_auto_filter3d_t *filter,
                                const oef_config_t *config,
                                oef_clock_fn clock, void *context)
{
    oef_auto_filter3d_t next = {0};
    oef_status_t st;
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    st = oef_init3d_ex(&next.filter, config);
    if (st != OEF_OK) return st;
    next.clock = clock;
    next.context = context;
    *filter = next;
    return OEF_OK;
}

/** @brief Scalar auto-update commits time only after core update succeeds. */
oef_status_t oef_auto_update_ex(oef_auto_filter_t *filter,
                                float input, float *output)
{
    oef_status_t st;
    double now;
    float dt;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!oef_config_valid(&filter->filter.config) ||
        filter->has_timestamp != filter->filter.initialized) return OEF_E_STATE;
    st = oef_auto_get_time(filter->clock, filter->context, &now);
    if (st != OEF_OK) return st;
    st = oef_auto_dt(filter->has_timestamp, filter->last_seconds, now, &dt);
    if (st != OEF_OK) return st;
    st = oef_update_ex(&filter->filter, input, dt, output);
    if (st != OEF_OK) return st;
    filter->last_seconds = now;
    filter->has_timestamp = true;
    return OEF_OK;
}

/** @brief 2D auto-update commits time only after successful vector update. */
oef_status_t oef_auto_update2d_ex(oef_auto_filter2d_t *filter,
                                  oef_vec2_t input, oef_vec2_t *output)
{
    oef_status_t st;
    double now;
    float dt;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!oef_config_valid(&filter->filter.config) ||
        filter->has_timestamp != filter->filter.initialized) return OEF_E_STATE;
    st = oef_auto_get_time(filter->clock, filter->context, &now);
    if (st != OEF_OK) return st;
    st = oef_auto_dt(filter->has_timestamp, filter->last_seconds, now, &dt);
    if (st != OEF_OK) return st;
    st = oef_update2d_ex(&filter->filter, input, dt, output);
    if (st != OEF_OK) return st;
    filter->last_seconds = now;
    filter->has_timestamp = true;
    return OEF_OK;
}

/** @brief 3D auto-update commits time only after successful vector update. */
oef_status_t oef_auto_update3d_ex(oef_auto_filter3d_t *filter,
                                  oef_vec3_t input, oef_vec3_t *output)
{
    oef_status_t st;
    double now;
    float dt;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!oef_config_valid(&filter->filter.config) ||
        filter->has_timestamp != filter->filter.initialized) return OEF_E_STATE;
    st = oef_auto_get_time(filter->clock, filter->context, &now);
    if (st != OEF_OK) return st;
    st = oef_auto_dt(filter->has_timestamp, filter->last_seconds, now, &dt);
    if (st != OEF_OK) return st;
    st = oef_update3d_ex(&filter->filter, input, dt, output);
    if (st != OEF_OK) return st;
    filter->last_seconds = now;
    filter->has_timestamp = true;
    return OEF_OK;
}

/** @brief Reset scalar history and timestamp, preserving callback. */
oef_status_t oef_auto_reset_ex(oef_auto_filter_t *filter)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_reset_ex(&filter->filter);
    if (st != OEF_OK) return st;
    filter->last_seconds = 0.0;
    filter->has_timestamp = false;
    return OEF_OK;
}

/** @brief Reset 2D history and timestamp, preserving callback. */
oef_status_t oef_auto_reset2d_ex(oef_auto_filter2d_t *filter)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_reset2d_ex(&filter->filter);
    if (st != OEF_OK) return st;
    filter->last_seconds = 0.0;
    filter->has_timestamp = false;
    return OEF_OK;
}

/** @brief Reset 3D history and timestamp, preserving callback. */
oef_status_t oef_auto_reset3d_ex(oef_auto_filter3d_t *filter)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_reset3d_ex(&filter->filter);
    if (st != OEF_OK) return st;
    filter->last_seconds = 0.0;
    filter->has_timestamp = false;
    return OEF_OK;
}
