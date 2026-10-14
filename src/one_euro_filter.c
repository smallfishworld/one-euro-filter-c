/* SPDX-License-Identifier: MIT */
/** @file one_euro_filter.c @brief Shared numerical core and dimension wrappers. */
#include "one_euro_filter.h"

#include <math.h>
#include <stddef.h>

/** @brief Mathematical constant used when mapping cutoff to low-pass alpha. */
#define OEF_TWO_PI 6.283185307179586476925286766559
/** @brief Maximum supported number of coordinates; no dynamic allocation. */
#define OEF_MAX_DIM 3U

/** @brief Low-pass coefficient in double precision; dt and cutoff are positive. */
static double oef_alpha(double cutoff_hz, double dt_seconds)
{
    double factor = OEF_TWO_PI * cutoff_hz * dt_seconds;
    return factor / (1.0 + factor);
}

/** @brief Validate configuration constraints. */
static oef_status_t oef_check_config(const oef_config_t *config)
{
    if (config == NULL) {
        return OEF_E_NULL;
    }
    if (!isfinite(config->min_cutoff_hz) ||
        config->min_cutoff_hz <= 0.0f ||
        !isfinite(config->beta) || config->beta < 0.0f ||
        !isfinite(config->derivative_cutoff_hz) ||
        config->derivative_cutoff_hz <= 0.0f) {
        return OEF_E_CONFIG;
    }
    return OEF_OK;
}

/**
 * @brief State transition for 1..3 coordinates.
 * @details Requires initialized filter state; a shared cutoff from the
 * Euclidean velocity norm preserves consistent smoothing across axes.
 */
static void oef_compute(const oef_config_t *config,
                        unsigned int dimensions,
                        const float *input,
                        const float *previous_raw,
                        const float *previous_filtered,
                        const float *previous_velocity,
                        float dt_seconds,
                        float *next_filtered,
                        float *next_velocity)
{
    double velocity[OEF_MAX_DIM];
    double derivative_alpha, speed = 0.0;
    double cutoff, alpha;
    unsigned int i;

    derivative_alpha = oef_alpha((double)config->derivative_cutoff_hz,
                                 (double)dt_seconds);

    for (i = 0U; i < dimensions; ++i) {
        /* Cast before subtraction: FLT_MAX - (-FLT_MAX) would overflow float. */
        double raw_speed = ((double)input[i] - (double)previous_raw[i]) /
                           (double)dt_seconds;
        velocity[i] = (double)previous_velocity[i] + derivative_alpha *
                      (raw_speed - (double)previous_velocity[i]);
        speed = hypot(speed, velocity[i]);
    }
    cutoff = (double)config->min_cutoff_hz + (double)config->beta * speed;
    alpha = oef_alpha(cutoff, (double)dt_seconds);

    for (i = 0U; i < dimensions; ++i) {
        double value = (double)previous_filtered[i] +
                       alpha * ((double)input[i] - (double)previous_filtered[i]);
        next_filtered[i] = (float)value;
        next_velocity[i] = (float)velocity[i];
    }
}

/** @brief Default configuration: conservative, tunable starting values. */
oef_config_t oef_config_default(void)
{
    const oef_config_t config = {0.8f, 0.5f, 1.0f};
    return config;
}

/** @brief Public bool validator for parameter ranges. */
bool oef_config_valid(const oef_config_t *config)
{
    return oef_check_config(config) == OEF_OK;
}

/** @brief Decode numeric error to a stable symbolic label. */
const char *oef_status_name(oef_status_t status)
{
    switch (status) {
    case OEF_OK: return "OEF_OK";
    case OEF_E_NULL: return "OEF_E_NULL";
    case OEF_E_CONFIG: return "OEF_E_CONFIG";
    case OEF_E_INPUT: return "OEF_E_INPUT";
    case OEF_E_TIME: return "OEF_E_TIME";
    default: return "OEF_UNKNOWN";
    }
}

/** @brief Decode numeric error to a static English diagnostic message. */
const char *oef_status_message(oef_status_t status)
{
    switch (status) {
    case OEF_OK: return "success";
    case OEF_E_NULL: return "required pointer is null";
    case OEF_E_CONFIG: return "invalid configuration parameter";
    case OEF_E_INPUT: return "input contains a non-finite value";
    case OEF_E_TIME: return "sample interval must be finite and positive";
    default: return "unknown status code";
    }
}

/** @brief Scalar initialization; NULL config selects the defaults. */
oef_status_t oef_init(oef_filter_t *filter, const oef_config_t *config)
{
    oef_config_t defaults;
    oef_status_t status;
    if (filter == NULL) return OEF_E_NULL;
    if (config == NULL) {
        defaults = oef_config_default();
        config = &defaults;
    }
    status = oef_check_config(config);
    if (status != OEF_OK) return status;
    filter->config = *config;
    filter->last_raw = 0.0f;
    filter->last_filtered = 0.0f;
    filter->last_velocity = 0.0f;
    filter->initialized = false;
    return OEF_OK;
}

/** @brief 2D initialization; NULL config selects the defaults. */
oef_status_t oef_init2d(oef_filter2d_t *filter, const oef_config_t *config)
{
    oef_config_t defaults;
    oef_status_t status;
    if (filter == NULL) return OEF_E_NULL;
    if (config == NULL) {
        defaults = oef_config_default();
        config = &defaults;
    }
    status = oef_check_config(config);
    if (status != OEF_OK) return status;
    filter->config = *config;
    filter->last_raw = (oef_vec2_t){0.0f, 0.0f};
    filter->last_filtered = (oef_vec2_t){0.0f, 0.0f};
    filter->last_velocity = (oef_vec2_t){0.0f, 0.0f};
    filter->initialized = false;
    return OEF_OK;
}

/** @brief 3D initialization; NULL config selects the defaults. */
oef_status_t oef_init3d(oef_filter3d_t *filter, const oef_config_t *config)
{
    oef_config_t defaults;
    oef_status_t status;
    if (filter == NULL) return OEF_E_NULL;
    if (config == NULL) {
        defaults = oef_config_default();
        config = &defaults;
    }
    status = oef_check_config(config);
    if (status != OEF_OK) return status;
    filter->config = *config;
    filter->last_raw = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->last_filtered = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->last_velocity = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->initialized = false;
    return OEF_OK;
}

/** @brief Change scalar tuning while preserving stored samples. */
oef_status_t oef_set_config(oef_filter_t *filter, const oef_config_t *config)
{
    oef_config_t defaults;
    if (filter == NULL) return OEF_E_NULL;
    if (config == NULL) {
        defaults = oef_config_default();
        config = &defaults;
    }
    if (oef_check_config(config) != OEF_OK) return OEF_E_CONFIG;
    filter->config = *config;
    return OEF_OK;
}

/** @brief Change 2D tuning while preserving stored samples. */
oef_status_t oef_set_config2d(oef_filter2d_t *filter, const oef_config_t *config)
{
    oef_config_t defaults;
    if (filter == NULL) return OEF_E_NULL;
    if (config == NULL) {
        defaults = oef_config_default();
        config = &defaults;
    }
    if (oef_check_config(config) != OEF_OK) return OEF_E_CONFIG;
    filter->config = *config;
    return OEF_OK;
}

/** @brief Change 3D tuning while preserving stored samples. */
oef_status_t oef_set_config3d(oef_filter3d_t *filter, const oef_config_t *config)
{
    oef_config_t defaults;
    if (filter == NULL) return OEF_E_NULL;
    if (config == NULL) {
        defaults = oef_config_default();
        config = &defaults;
    }
    if (oef_check_config(config) != OEF_OK) return OEF_E_CONFIG;
    filter->config = *config;
    return OEF_OK;
}

/** @brief Reset scalar history, keeping the configuration. */
oef_status_t oef_reset(oef_filter_t *filter)
{
    if (filter == NULL) return OEF_E_NULL;
    filter->last_raw = 0.0f;
    filter->last_filtered = 0.0f;
    filter->last_velocity = 0.0f;
    filter->initialized = false;
    return OEF_OK;
}

/** @brief Reset 2D history, keeping the configuration. */
oef_status_t oef_reset2d(oef_filter2d_t *filter)
{
    if (filter == NULL) return OEF_E_NULL;
    filter->last_raw = (oef_vec2_t){0.0f, 0.0f};
    filter->last_filtered = (oef_vec2_t){0.0f, 0.0f};
    filter->last_velocity = (oef_vec2_t){0.0f, 0.0f};
    filter->initialized = false;
    return OEF_OK;
}

/** @brief Reset 3D history, keeping the configuration. */
oef_status_t oef_reset3d(oef_filter3d_t *filter)
{
    if (filter == NULL) return OEF_E_NULL;
    filter->last_raw = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->last_filtered = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->last_velocity = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->initialized = false;
    return OEF_OK;
}

/** @brief Scalar update; first sample passes through, later samples smooth. */
oef_status_t oef_update(oef_filter_t *filter, float input,
                        float dt_seconds, float *output)
{
    float next[1], velocity[1];
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!isfinite(input)) return OEF_E_INPUT;
    if (!isfinite(dt_seconds) || dt_seconds <= 0.0f) return OEF_E_TIME;
    if (!filter->initialized) {
        filter->last_raw = input;
        filter->last_filtered = input;
        filter->last_velocity = 0.0f;
        filter->initialized = true;
        *output = input;
        return OEF_OK;
    }
    oef_compute(&filter->config, 1U, &input, &filter->last_raw,
                &filter->last_filtered, &filter->last_velocity,
                dt_seconds, next, velocity);
    filter->last_raw = input;
    filter->last_filtered = next[0];
    filter->last_velocity = velocity[0];
    *output = next[0];
    return OEF_OK;
}

/** @brief 2D update; one shared alpha computed by vector speed. */
oef_status_t oef_update2d(oef_filter2d_t *filter, oef_vec2_t input,
                          float dt_seconds, oef_vec2_t *output)
{
    float next[2], velocity[2];
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!isfinite(input.x) || !isfinite(input.y)) return OEF_E_INPUT;
    if (!isfinite(dt_seconds) || dt_seconds <= 0.0f) return OEF_E_TIME;
    if (!filter->initialized) {
        filter->last_raw = input;
        filter->last_filtered = input;
        filter->last_velocity = (oef_vec2_t){0.0f, 0.0f};
        filter->initialized = true;
        *output = input;
        return OEF_OK;
    }
    {
        const float in[2] = {input.x, input.y};
        const float old_raw[2] = {filter->last_raw.x, filter->last_raw.y};
        const float old_out[2] = {filter->last_filtered.x, filter->last_filtered.y};
        const float old_vel[2] = {filter->last_velocity.x, filter->last_velocity.y};
        oef_compute(&filter->config, 2U, in, old_raw, old_out, old_vel,
                    dt_seconds, next, velocity);
    }
    filter->last_raw = input;
    filter->last_filtered.x = next[0];
    filter->last_filtered.y = next[1];
    filter->last_velocity.x = velocity[0];
    filter->last_velocity.y = velocity[1];
    output->x = next[0];
    output->y = next[1];
    return OEF_OK;
}

/** @brief 3D update; one shared alpha computed by vector speed. */
oef_status_t oef_update3d(oef_filter3d_t *filter, oef_vec3_t input,
                          float dt_seconds, oef_vec3_t *output)
{
    float next[3], velocity[3];
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    if (!isfinite(input.x) || !isfinite(input.y) || !isfinite(input.z)) {
        return OEF_E_INPUT;
    }
    if (!isfinite(dt_seconds) || dt_seconds <= 0.0f) return OEF_E_TIME;
    if (!filter->initialized) {
        filter->last_raw = input;
        filter->last_filtered = input;
        filter->last_velocity = (oef_vec3_t){0.0f, 0.0f, 0.0f};
        filter->initialized = true;
        *output = input;
        return OEF_OK;
    }
    {
        const float in[3] = {input.x, input.y, input.z};
        const float old_raw[3] = {filter->last_raw.x, filter->last_raw.y,
                                  filter->last_raw.z};
        const float old_out[3] = {filter->last_filtered.x, filter->last_filtered.y,
                                  filter->last_filtered.z};
        const float old_vel[3] = {filter->last_velocity.x, filter->last_velocity.y,
                                  filter->last_velocity.z};
        oef_compute(&filter->config, 3U, in, old_raw, old_out, old_vel,
                    dt_seconds, next, velocity);
    }
    filter->last_raw = input;
    filter->last_filtered.x = next[0];
    filter->last_filtered.y = next[1];
    filter->last_filtered.z = next[2];
    filter->last_velocity.x = velocity[0];
    filter->last_velocity.y = velocity[1];
    filter->last_velocity.z = velocity[2];
    output->x = next[0];
    output->y = next[1];
    output->z = next[2];
    return OEF_OK;
}
