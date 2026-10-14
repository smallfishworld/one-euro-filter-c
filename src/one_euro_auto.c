/* SPDX-License-Identifier: MIT */
/** @file one_euro_auto.c @brief Automatic timing adapters for scalar/vectors. */
#include "one_euro_filter.h"

#include <math.h>
#include <stddef.h>

/**
 * @brief Read the active clock, preferring a user callback.
 * @details NULL clock selects the built-in monotonic clock. An unwritable
 * or unusable backend eventually surfaces as OEF_E_TIME via the dt check.
 */
static double oef_auto_now(oef_clock_fn clock, void *context)
{
    return clock != NULL ? clock(context) : oef_clock_monotonic();
}

/** @brief Scalar adapter initialization; NULL config selects the defaults. */
oef_status_t oef_auto_init(oef_auto_filter_t *filter,
                          const oef_config_t *config,
                          oef_clock_fn clock, void *context)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_init(&filter->filter, config);
    if (st != OEF_OK) return st;
    filter->clock = clock;
    filter->context = context;
    filter->last_seconds = 0.0;
    return OEF_OK;
}

/** @brief 2D adapter initialization; NULL config selects the defaults. */
oef_status_t oef_auto_init2d(oef_auto_filter2d_t *filter,
                            const oef_config_t *config,
                            oef_clock_fn clock, void *context)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_init2d(&filter->filter, config);
    if (st != OEF_OK) return st;
    filter->clock = clock;
    filter->context = context;
    filter->last_seconds = 0.0;
    return OEF_OK;
}

/** @brief 3D adapter initialization; NULL config selects the defaults. */
oef_status_t oef_auto_init3d(oef_auto_filter3d_t *filter,
                            const oef_config_t *config,
                            oef_clock_fn clock, void *context)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_init3d(&filter->filter, config);
    if (st != OEF_OK) return st;
    filter->clock = clock;
    filter->context = context;
    filter->last_seconds = 0.0;
    return OEF_OK;
}

/** @brief Scalar auto-update; commits timestamp only after core succeeds. */
oef_status_t oef_auto_update(oef_auto_filter_t *filter,
                            float input, float *output)
{
    double now;
    oef_status_t st;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!isfinite(input)) return OEF_E_INPUT;
    now = oef_auto_now(filter->clock, filter->context);
    if (filter->filter.initialized) {
        float dt = (float)(now - filter->last_seconds);
        if (!isfinite(dt) || dt <= 0.0f) return OEF_E_TIME;
        st = oef_update(&filter->filter, input, dt, output);
        if (st != OEF_OK) return st;
        filter->last_seconds = now;
    } else {
        /* First sample: passes through and only stores the timestamp. */
        st = oef_update(&filter->filter, input, 1.0f, output);
        if (st != OEF_OK) return st;
        filter->last_seconds = now;
    }
    return OEF_OK;
}

/** @brief 2D auto-update; commits timestamp only after core succeeds. */
oef_status_t oef_auto_update2d(oef_auto_filter2d_t *filter,
                              oef_vec2_t input, oef_vec2_t *output)
{
    double now;
    oef_status_t st;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!isfinite(input.x) || !isfinite(input.y)) return OEF_E_INPUT;
    now = oef_auto_now(filter->clock, filter->context);
    if (filter->filter.initialized) {
        float dt = (float)(now - filter->last_seconds);
        if (!isfinite(dt) || dt <= 0.0f) return OEF_E_TIME;
        st = oef_update2d(&filter->filter, input, dt, output);
        if (st != OEF_OK) return st;
        filter->last_seconds = now;
    } else {
        st = oef_update2d(&filter->filter, input, 1.0f, output);
        if (st != OEF_OK) return st;
        filter->last_seconds = now;
    }
    return OEF_OK;
}

/** @brief 3D auto-update; commits timestamp only after core succeeds. */
oef_status_t oef_auto_update3d(oef_auto_filter3d_t *filter,
                              oef_vec3_t input, oef_vec3_t *output)
{
    double now;
    oef_status_t st;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!isfinite(input.x) || !isfinite(input.y) || !isfinite(input.z)) {
        return OEF_E_INPUT;
    }
    now = oef_auto_now(filter->clock, filter->context);
    if (filter->filter.initialized) {
        float dt = (float)(now - filter->last_seconds);
        if (!isfinite(dt) || dt <= 0.0f) return OEF_E_TIME;
        st = oef_update3d(&filter->filter, input, dt, output);
        if (st != OEF_OK) return st;
        filter->last_seconds = now;
    } else {
        st = oef_update3d(&filter->filter, input, 1.0f, output);
        if (st != OEF_OK) return st;
        filter->last_seconds = now;
    }
    return OEF_OK;
}

/** @brief Reset scalar history and timestamp, preserving callback. */
oef_status_t oef_auto_reset(oef_auto_filter_t *filter)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_reset(&filter->filter);
    if (st != OEF_OK) return st;
    filter->last_seconds = 0.0;
    return OEF_OK;
}

/** @brief Reset 2D history and timestamp, preserving callback. */
oef_status_t oef_auto_reset2d(oef_auto_filter2d_t *filter)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_reset2d(&filter->filter);
    if (st != OEF_OK) return st;
    filter->last_seconds = 0.0;
    return OEF_OK;
}

/** @brief Reset 3D history and timestamp, preserving callback. */
oef_status_t oef_auto_reset3d(oef_auto_filter3d_t *filter)
{
    oef_status_t st;
    if (filter == NULL) return OEF_E_NULL;
    st = oef_reset3d(&filter->filter);
    if (st != OEF_OK) return st;
    filter->last_seconds = 0.0;
    return OEF_OK;
}
