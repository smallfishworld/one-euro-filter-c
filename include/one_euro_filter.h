/* SPDX-License-Identifier: MIT */
#ifndef ONE_EURO_FILTER_H
#define ONE_EURO_FILTER_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Cutoff frequencies use Hz. Beta depends on the input coordinate unit:
 * cutoff_hz = min_cutoff_hz + beta * abs(velocity_in_units_per_second). */
typedef struct {
    float min_cutoff_hz;        /* >0; lower = smoother at rest */
    float beta;                 /* >=0; higher = faster motion response */
    float derivative_cutoff_hz; /* >0; velocity estimate smoothing */
    float sample_frequency_hz; /* >0; used by oef_step* only */
    float max_cutoff_hz;        /* 0 = unlimited, otherwise >= min_cutoff */
} oef_config_t;

typedef struct {
    float x, y;
} oef_vec2_t;
typedef struct {
    float x, y, z;
} oef_vec3_t;

typedef struct {
    oef_config_t config;
    float last_raw, last_filtered, last_velocity;
    bool initialized;
} oef_filter_t;
typedef struct {
    oef_config_t config;
    oef_vec2_t last_raw, last_filtered, last_velocity;
    bool initialized;
} oef_filter2d_t;
typedef struct {
    oef_config_t config;
    oef_vec3_t last_raw, last_filtered, last_velocity;
    bool initialized;
} oef_filter3d_t;

#define OEF_VERSION_MAJOR 1
#define OEF_VERSION_MINOR 0
#define OEF_VERSION_PATCH 0

/* Default: min=0.8 Hz, beta=0.5, derivative=1 Hz, sample=25 Hz,
 * no maximum cutoff; starting values, not universal calibration. */
oef_config_t oef_config_default(void);
bool oef_config_valid(const oef_config_t *config);

/* Initialization clears history. No heap, OS API, or global mutable state. */
bool oef_init(oef_filter_t *filter, const oef_config_t *config);
bool oef_init2d(oef_filter2d_t *filter, const oef_config_t *config);
bool oef_init3d(oef_filter3d_t *filter, const oef_config_t *config);

/* Runtime reconfiguration retains history; invalid configs are rejected. */
bool oef_set_config(oef_filter_t *filter, const oef_config_t *config);
bool oef_set_config2d(oef_filter2d_t *filter, const oef_config_t *config);
bool oef_set_config3d(oef_filter3d_t *filter, const oef_config_t *config);

/* Reset history, keep configuration. */
void oef_reset(oef_filter_t *filter);
void oef_reset2d(oef_filter2d_t *filter);
void oef_reset3d(oef_filter3d_t *filter);

/* Actual elapsed time in seconds, positive and finite. First valid sample
 * passes through. Invalid input/dt/config returns false without changing
 * state or output. 2D/3D use shared alpha from vector velocity magnitude.
 * Instances are not internally synchronized. */
bool oef_update(oef_filter_t *filter, float input, float dt_seconds,
                float *output);
bool oef_update2d(oef_filter2d_t *filter, oef_vec2_t input, float dt_seconds,
                  oef_vec2_t *output);
bool oef_update3d(oef_filter3d_t *filter, oef_vec3_t input, float dt_seconds,
                  oef_vec3_t *output);

/* Fixed-rate shortcuts using config.sample_frequency_hz. */
bool oef_step(oef_filter_t *filter, float input, float *output);
bool oef_step2d(oef_filter2d_t *filter, oef_vec2_t input, oef_vec2_t *output);
bool oef_step3d(oef_filter3d_t *filter, oef_vec3_t input, oef_vec3_t *output);

#ifdef __cplusplus
}
#endif
#endif /* ONE_EURO_FILTER_H */
