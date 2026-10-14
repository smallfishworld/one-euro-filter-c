/* SPDX-License-Identifier: MIT */
/** @file one_euro_filter.c @brief Shared numerical core and dimension wrappers. */
#include "one_euro_filter.h"

#include <float.h>
#include <math.h>
#include <string.h>

/** @brief Mathematical constant used when mapping cutoff to low-pass alpha. */
#define OEF_TWO_PI 6.283185307179586476925286766559
/** @brief Maximum supported number of coordinates; no dynamic allocation. */
#define OEF_MAX_DIM 3U

/** @brief Check that a candidate sample contains only finite coordinates. */
static bool oef_finite_array(const float *a, unsigned int dimensions)
{
    unsigned int i;
    for (i = 0U; i < dimensions; ++i) {
        if (!isfinite(a[i])) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Calculate low-pass coefficient in double precision.
 * @details dt and cutoff are strictly positive. The double intermediate
 * prevents spurious float overflow when an otherwise-valid value is large.
 */
static double oef_alpha(double cutoff_hz, double dt_seconds)
{
    double factor = OEF_TWO_PI * cutoff_hz * dt_seconds;
    return factor / (1.0 + factor);
}

/** @brief Validate configuration separately from a filter's current state. */
static oef_status_t oef_check_config(const oef_config_t *config)
{
    if (config == NULL) {
        return OEF_E_NULL;
    }
    if (!isfinite(config->min_cutoff_hz) ||
        config->min_cutoff_hz <= 0.0f ||
        !isfinite(config->beta) || config->beta < 0.0f ||
        !isfinite(config->derivative_cutoff_hz) ||
        config->derivative_cutoff_hz <= 0.0f ||
        !isfinite(config->max_cutoff_hz) ||
        config->max_cutoff_hz < 0.0f ||
        (config->max_cutoff_hz > 0.0f &&
         config->max_cutoff_hz < config->min_cutoff_hz)) {
        return OEF_E_CONFIG;
    }
    return OEF_OK;
}

/**
 * @brief Validate retained history and live configuration.
 * @details A false initialized flag is normal before the first successful
 * sample; history is then ignored. This method cannot make using a truly
 * uninitialized C object safe: calling init first is a required contract.
 */
static bool oef_history_valid(const oef_config_t *config, bool initialized,
                              const float *raw, const float *filtered,
                              const float *velocity, unsigned int dimensions)
{
    if (oef_check_config(config) != OEF_OK) {
        return false;
    }
    return !initialized ||
           (oef_finite_array(raw, dimensions) &&
            oef_finite_array(filtered, dimensions) &&
            oef_finite_array(velocity, dimensions));
}

/**
 * @brief Pure state transition for 1..3 coordinates.
 * @details Verifies all calculations before the caller commits new history.
 * A shared cutoff from the Euclidean velocity norm preserves consistent
 * smoothing strength across vector axes.
 */
static oef_status_t oef_compute(const oef_config_t *config,
                                bool initialized,
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

    if (!oef_finite_array(input, dimensions)) {
        return OEF_E_INPUT;
    }
    if (!isfinite(dt_seconds) || dt_seconds <= 0.0f) {
        return OEF_E_TIME;
    }
    if (!oef_history_valid(config, initialized, previous_raw,
                           previous_filtered, previous_velocity, dimensions)) {
        return OEF_E_STATE;
    }
    if (!initialized) {
        for (i = 0U; i < dimensions; ++i) {
            next_filtered[i] = input[i];
            next_velocity[i] = 0.0f;
        }
        return OEF_OK;
    }

    derivative_alpha = oef_alpha((double)config->derivative_cutoff_hz,
                                 (double)dt_seconds);
    if (!isfinite(derivative_alpha)) {
        return OEF_E_RANGE;
    }

    for (i = 0U; i < dimensions; ++i) {
        /* Cast before subtraction: FLT_MAX - (-FLT_MAX) would overflow in float. */
        double raw_speed = ((double)input[i] - (double)previous_raw[i]) /
                           (double)dt_seconds;
        velocity[i] = (double)previous_velocity[i] + derivative_alpha *
                       (raw_speed - (double)previous_velocity[i]);
        if (!isfinite(velocity[i]) || fabs(velocity[i]) > (double)FLT_MAX) {
            return OEF_E_RANGE;
        }
        speed = hypot(speed, velocity[i]);
    }
    cutoff = (double)config->min_cutoff_hz +
             (double)config->beta * speed;
    if (config->max_cutoff_hz > 0.0f &&
        cutoff > (double)config->max_cutoff_hz) {
        cutoff = (double)config->max_cutoff_hz;
    }
    alpha = oef_alpha(cutoff, (double)dt_seconds);
    if (!isfinite(speed) || !isfinite(cutoff) ||
        !isfinite(alpha) || alpha < 0.0 || alpha > 1.0) {
        return OEF_E_RANGE;
    }

    for (i = 0U; i < dimensions; ++i) {
        double value = (double)previous_filtered[i] +
                       alpha * ((double)input[i] - (double)previous_filtered[i]);
        if (!isfinite(value) || fabs(value) > (double)FLT_MAX) {
            return OEF_E_RANGE;
        }
        next_filtered[i] = (float)value;
        next_velocity[i] = (float)velocity[i];
        if (!isfinite(next_filtered[i]) || !isfinite(next_velocity[i])) {
            return OEF_E_RANGE;
        }
    }
    return OEF_OK;
}

/** @brief Default configuration: conservative, tunable starting values. */
oef_config_t oef_config_default(void)
{
    const oef_config_t config = {0.8f, 0.5f, 1.0f, 0.0f};
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
    case OEF_E_STATE: return "OEF_E_STATE";
    case OEF_E_RANGE: return "OEF_E_RANGE";
    case OEF_E_CLOCK: return "OEF_E_CLOCK";
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
    case OEF_E_STATE: return "filter state or active configuration is invalid";
    case OEF_E_RANGE: return "numeric result cannot be represented";
    case OEF_E_CLOCK: return "monotonic clock failed or returned an invalid timestamp";
    default: return "unknown status code";
    }
}

/** @brief Transactional scalar initialization; validates before writing. */
oef_status_t oef_init_ex(oef_filter_t *filter, const oef_config_t *config)
{
    oef_status_t status;
    if (filter == NULL || config == NULL) {
        return OEF_E_NULL;
    }
    status = oef_check_config(config);
    if (status != OEF_OK) {
        return status;
    }
    /* Copy before clearing in case config aliases filter->config. */
    {
        oef_config_t copy = *config;
        memset(filter, 0, sizeof(*filter));
        filter->config = copy;
    }
    return OEF_OK;
}

/** @brief Transactional 2D initialization; validates before writing. */
oef_status_t oef_init2d_ex(oef_filter2d_t *filter, const oef_config_t *config)
{
    oef_status_t status;
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    status = oef_check_config(config);
    if (status != OEF_OK) return status;
    {
        oef_config_t copy = *config;
        memset(filter, 0, sizeof(*filter));
        filter->config = copy;
    }
    return OEF_OK;
}

/** @brief Transactional 3D initialization; validates before writing. */
oef_status_t oef_init3d_ex(oef_filter3d_t *filter, const oef_config_t *config)
{
    oef_status_t status;
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    status = oef_check_config(config);
    if (status != OEF_OK) return status;
    {
        oef_config_t copy = *config;
        memset(filter, 0, sizeof(*filter));
        filter->config = copy;
    }
    return OEF_OK;
}

/** @brief Change scalar tuning while preserving validated stored samples. */
oef_status_t oef_set_config_ex(oef_filter_t *filter, const oef_config_t *config)
{
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    if (oef_check_config(config) != OEF_OK) return OEF_E_CONFIG;
    if (!oef_history_valid(&filter->config, filter->initialized,
                           &filter->last_raw, &filter->last_filtered,
                           &filter->last_velocity, 1U)) return OEF_E_STATE;
    filter->config = *config;
    return OEF_OK;
}

/** @brief Change 2D tuning while preserving validated stored samples. */
oef_status_t oef_set_config2d_ex(oef_filter2d_t *filter, const oef_config_t *config)
{
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    if (oef_check_config(config) != OEF_OK) return OEF_E_CONFIG;
    {
        const float raw[2] = {filter->last_raw.x, filter->last_raw.y};
        const float out[2] = {filter->last_filtered.x, filter->last_filtered.y};
        const float vel[2] = {filter->last_velocity.x, filter->last_velocity.y};
        if (!oef_history_valid(&filter->config, filter->initialized,
                               raw, out, vel, 2U)) return OEF_E_STATE;
    }
    filter->config = *config;
    return OEF_OK;
}

/** @brief Change 3D tuning while preserving validated stored samples. */
oef_status_t oef_set_config3d_ex(oef_filter3d_t *filter, const oef_config_t *config)
{
    if (filter == NULL || config == NULL) return OEF_E_NULL;
    if (oef_check_config(config) != OEF_OK) return OEF_E_CONFIG;
    {
        const float raw[3] = {filter->last_raw.x, filter->last_raw.y,
                              filter->last_raw.z};
        const float out[3] = {filter->last_filtered.x, filter->last_filtered.y,
                              filter->last_filtered.z};
        const float vel[3] = {filter->last_velocity.x, filter->last_velocity.y,
                              filter->last_velocity.z};
        if (!oef_history_valid(&filter->config, filter->initialized,
                               raw, out, vel, 3U)) return OEF_E_STATE;
    }
    filter->config = *config;
    return OEF_OK;
}

/** @brief Reset only the scalar history after validating configuration. */
oef_status_t oef_reset_ex(oef_filter_t *filter)
{
    if (filter == NULL) return OEF_E_NULL;
    if (oef_check_config(&filter->config) != OEF_OK) return OEF_E_STATE;
    filter->last_raw = 0.0f;
    filter->last_filtered = 0.0f;
    filter->last_velocity = 0.0f;
    filter->initialized = false;
    return OEF_OK;
}

/** @brief Reset only the 2D history after validating configuration. */
oef_status_t oef_reset2d_ex(oef_filter2d_t *filter)
{
    if (filter == NULL) return OEF_E_NULL;
    if (oef_check_config(&filter->config) != OEF_OK) return OEF_E_STATE;
    filter->last_raw = (oef_vec2_t){0.0f, 0.0f};
    filter->last_filtered = (oef_vec2_t){0.0f, 0.0f};
    filter->last_velocity = (oef_vec2_t){0.0f, 0.0f};
    filter->initialized = false;
    return OEF_OK;
}

/** @brief Reset only the 3D history after validating configuration. */
oef_status_t oef_reset3d_ex(oef_filter3d_t *filter)
{
    if (filter == NULL) return OEF_E_NULL;
    if (oef_check_config(&filter->config) != OEF_OK) return OEF_E_STATE;
    filter->last_raw = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->last_filtered = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->last_velocity = (oef_vec3_t){0.0f, 0.0f, 0.0f};
    filter->initialized = false;
    return OEF_OK;
}

/** @brief Scalar update; commit only after shared engine succeeds. */
oef_status_t oef_update_ex(oef_filter_t *filter, float input,
                           float dt_seconds, float *output)
{
    float next[OEF_MAX_DIM], velocity[OEF_MAX_DIM];
    oef_status_t status;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    status = oef_compute(&filter->config, filter->initialized, 1U,
                         &input, &filter->last_raw, &filter->last_filtered,
                         &filter->last_velocity, dt_seconds, next, velocity);
    if (status != OEF_OK) return status;
    filter->last_raw = input;
    filter->last_filtered = next[0];
    filter->last_velocity = velocity[0];
    filter->initialized = true;
    *output = next[0];
    return OEF_OK;
}

/** @brief 2D update; one shared alpha computed by vector speed. */
oef_status_t oef_update2d_ex(oef_filter2d_t *filter, oef_vec2_t input,
                             float dt_seconds, oef_vec2_t *output)
{
    float next[OEF_MAX_DIM], velocity[OEF_MAX_DIM];
    oef_status_t status;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    {
        const float in[2] = {input.x, input.y};
        const float old_raw[2] = {filter->last_raw.x, filter->last_raw.y};
        const float old_out[2] = {filter->last_filtered.x, filter->last_filtered.y};
        const float old_vel[2] = {filter->last_velocity.x, filter->last_velocity.y};
        status = oef_compute(&filter->config, filter->initialized, 2U,
                             in, old_raw, old_out, old_vel,
                             dt_seconds, next, velocity);
    }
    if (status != OEF_OK) return status;
    filter->last_raw = input;
    filter->last_filtered.x = next[0];
    filter->last_filtered.y = next[1];
    filter->last_velocity.x = velocity[0];
    filter->last_velocity.y = velocity[1];
    filter->initialized = true;
    output->x = next[0];
    output->y = next[1];
    return OEF_OK;
}

/** @brief 3D update; one shared alpha computed by vector speed. */
oef_status_t oef_update3d_ex(oef_filter3d_t *filter, oef_vec3_t input,
                             float dt_seconds, oef_vec3_t *output)
{
    float next[OEF_MAX_DIM], velocity[OEF_MAX_DIM];
    oef_status_t status;
    if (filter == NULL || output == NULL) return OEF_E_NULL;
    {
        const float in[3] = {input.x, input.y, input.z};
        const float old_raw[3] = {filter->last_raw.x, filter->last_raw.y,
                                  filter->last_raw.z};
        const float old_out[3] = {filter->last_filtered.x, filter->last_filtered.y,
                                  filter->last_filtered.z};
        const float old_vel[3] = {filter->last_velocity.x, filter->last_velocity.y,
                                  filter->last_velocity.z};
        status = oef_compute(&filter->config, filter->initialized, 3U,
                             in, old_raw, old_out, old_vel,
                             dt_seconds, next, velocity);
    }
    if (status != OEF_OK) return status;
    filter->last_raw = input;
    filter->last_filtered.x = next[0];
    filter->last_filtered.y = next[1];
    filter->last_filtered.z = next[2];
    filter->last_velocity.x = velocity[0];
    filter->last_velocity.y = velocity[1];
    filter->last_velocity.z = velocity[2];
    filter->initialized = true;
    output->x = next[0];
    output->y = next[1];
    output->z = next[2];
    return OEF_OK;
}

/** @brief Boolean convenience wrappers for manual-dt API. */
bool oef_init(oef_filter_t *f, const oef_config_t *c)
{ return oef_init_ex(f, c) == OEF_OK; }
/** @brief Compatibility wrapper for 2D initialization. */
bool oef_init2d(oef_filter2d_t *f, const oef_config_t *c)
{ return oef_init2d_ex(f, c) == OEF_OK; }
/** @brief Compatibility wrapper for 3D initialization. */
bool oef_init3d(oef_filter3d_t *f, const oef_config_t *c)
{ return oef_init3d_ex(f, c) == OEF_OK; }
/** @brief Compatibility wrapper for scalar reconfiguration. */
bool oef_set_config(oef_filter_t *f, const oef_config_t *c)
{ return oef_set_config_ex(f, c) == OEF_OK; }
/** @brief Compatibility wrapper for 2D reconfiguration. */
bool oef_set_config2d(oef_filter2d_t *f, const oef_config_t *c)
{ return oef_set_config2d_ex(f, c) == OEF_OK; }
/** @brief Compatibility wrapper for 3D reconfiguration. */
bool oef_set_config3d(oef_filter3d_t *f, const oef_config_t *c)
{ return oef_set_config3d_ex(f, c) == OEF_OK; }
/** @brief Compatibility wrapper for scalar reset; ignores status. */
void oef_reset(oef_filter_t *f)
{ (void)oef_reset_ex(f); }
/** @brief Compatibility wrapper for 2D reset; ignores status. */
void oef_reset2d(oef_filter2d_t *f)
{ (void)oef_reset2d_ex(f); }
/** @brief Compatibility wrapper for 3D reset; ignores status. */
void oef_reset3d(oef_filter3d_t *f)
{ (void)oef_reset3d_ex(f); }
/** @brief Compatibility wrapper for scalar variable-rate update. */
bool oef_update(oef_filter_t *f, float x, float dt, float *out)
{ return oef_update_ex(f, x, dt, out) == OEF_OK; }
/** @brief Compatibility wrapper for 2D variable-rate update. */
bool oef_update2d(oef_filter2d_t *f, oef_vec2_t x, float dt, oef_vec2_t *out)
{ return oef_update2d_ex(f, x, dt, out) == OEF_OK; }
/** @brief Compatibility wrapper for 3D variable-rate update. */
bool oef_update3d(oef_filter3d_t *f, oef_vec3_t x, float dt, oef_vec3_t *out)
{ return oef_update3d_ex(f, x, dt, out) == OEF_OK; }
