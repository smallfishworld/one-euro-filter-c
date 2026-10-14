/* SPDX-License-Identifier: MIT */
/**
 * @file one_euro_filter.h
 * @brief Allocation-free 1D/2D/3D One Euro Filter (C99).
 *
 * @details Cutoff frequencies are in Hz, time intervals in seconds.
 * Call an init function before using a filter instance. Concurrent access
 * to the SAME instance requires caller synchronization.
 */
#ifndef ONE_EURO_FILTER_H
#define ONE_EURO_FILTER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup oef_api One Euro Filter public API
 *  @brief Stateful filters for sampled scalar and vector signals.
 *  @{ */

/** @brief Semantic-version major number. */
#define OEF_VERSION_MAJOR 3
/** @brief Semantic-version minor number. */
#define OEF_VERSION_MINOR 0
/** @brief Semantic-version patch number. */
#define OEF_VERSION_PATCH 0

/**
 * @brief Operation result. Success is zero; errors are stable negative values.
 * @details Library-defined; MUST NOT be treated as POSIX errno.
 */
typedef enum {
    OEF_OK       = 0, /**< Operation completed successfully. */
    OEF_E_NULL   = -1, /**< Required pointer argument is NULL. */
    OEF_E_CONFIG = -2, /**< Configuration contains invalid values. */
    OEF_E_INPUT  = -3, /**< Input sample contains NaN or infinity. */
    OEF_E_TIME   = -4  /**< Sample interval is zero, negative or non-finite. */
} oef_status_t;

/**
 * @brief Tunable parameters for one filter instance.
 * @details Cutoff frequencies are positive and finite. Speed is measured in
 * input units per second, so beta depends on input units.
 * Use oef_config_default() as the recommended starting point.
 */
typedef struct {
    float min_cutoff_hz;        /**< Minimum low-pass cutoff, >0 Hz. */
    float beta;                 /**< Velocity sensitivity, finite and >=0. */
    float derivative_cutoff_hz; /**< Velocity low-pass cutoff, >0 Hz. */
} oef_config_t;

/** @brief Two-dimensional coordinate with matching units on both axes. */
typedef struct {
    float x; /**< First coordinate, finite. */
    float y; /**< Second coordinate, finite. */
} oef_vec2_t;

/** @brief Three-dimensional coordinate with matching units on all axes. */
typedef struct {
    float x; /**< First coordinate, finite. */
    float y; /**< Second coordinate, finite. */
    float z; /**< Third coordinate, finite. */
} oef_vec3_t;

/**
 * @brief Stateful scalar filter; application owns its lifetime and storage.
 * @warning Treat fields other than config as read-only.
 */
typedef struct {
    oef_config_t config; /**< Current validated configuration. */
    float last_raw;      /**< Previous unfiltered input. */
    float last_filtered; /**< Previous output. */
    float last_velocity; /**< Previous smoothed velocity in units/s. */
    bool initialized;    /**< True after the first successful sample. */
} oef_filter_t;

/** @brief Stateful 2D vector filter using one shared adaptive cutoff. */
typedef struct {
    oef_config_t config;      /**< Current validated configuration. */
    oef_vec2_t last_raw;      /**< Previous unfiltered vector. */
    oef_vec2_t last_filtered; /**< Previous filtered vector. */
    oef_vec2_t last_velocity; /**< Previous filtered velocity (units/s). */
    bool initialized;         /**< True after the first successful vector. */
} oef_filter2d_t;

/** @brief Stateful 3D vector filter using one shared adaptive cutoff. */
typedef struct {
    oef_config_t config;      /**< Current validated configuration. */
    oef_vec3_t last_raw;      /**< Previous unfiltered vector. */
    oef_vec3_t last_filtered; /**< Previous filtered vector. */
    oef_vec3_t last_velocity; /**< Previous filtered velocity (units/s). */
    bool initialized;         /**< True after the first successful vector. */
} oef_filter3d_t;

/**
 * @brief Obtain initial parameter values.
 * @return Config (0.8 Hz minimum, beta=0.5, 1.0 Hz derivative).
 * @note Tune the values for the physical units and noise of each application.
 */
oef_config_t oef_config_default(void);

/**
 * @brief Check all configuration constraints.
 * @param[in] config Candidate config, or NULL.
 * @return true if valid; false for NULL or any invalid field.
 */
bool oef_config_valid(const oef_config_t *config);

/**
 * @brief Convert a status into its stable symbolic name.
 * @param[in] status Library status or arbitrary integer cast to oef_status_t.
 * @return Static string; unknown statuses return "OEF_UNKNOWN".
 */
const char *oef_status_name(oef_status_t status);

/**
 * @brief Obtain a human-readable explanation of a status.
 * @param[in] status Library status, including unknown values.
 * @return Static English string; never NULL. No heap allocation.
 */
const char *oef_status_message(oef_status_t status);

/**
 * @brief Initialize a scalar filter and discard its old history.
 * @param[out] filter Caller-owned instance, non-NULL.
 * @param[in] config Parameters, or NULL for oef_config_default().
 * @retval OEF_OK Initialized; first update returns input unchanged.
 * @retval OEF_E_NULL Filter pointer is NULL.
 * @retval OEF_E_CONFIG Invalid configuration; filter is not modified.
 */
oef_status_t oef_init(oef_filter_t *filter, const oef_config_t *config);

/** @brief Initialize 2D filter; see oef_init() for contracts.
 * @param[out] filter Initialized caller-owned filter.
 * @param[in] config Valid parameters, or NULL for defaults.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG.
 */
oef_status_t oef_init2d(oef_filter2d_t *filter, const oef_config_t *config);

/** @brief Initialize 3D filter; see oef_init() for contracts.
 * @param[out] filter Initialized caller-owned filter.
 * @param[in] config Valid parameters, or NULL for defaults.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG.
 */
oef_status_t oef_init3d(oef_filter3d_t *filter, const oef_config_t *config);

/**
 * @brief Change scalar tuning while retaining filter history.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] config New valid parameter set, or NULL for defaults.
 * @retval OEF_OK Configuration applied.
 * @retval OEF_E_NULL NULL filter pointer.
 * @retval OEF_E_CONFIG New configuration is invalid (no mutation).
 */
oef_status_t oef_set_config(oef_filter_t *filter, const oef_config_t *config);

/** @brief Reconfigure initialized 2D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @param[in] config Proposed parameters, or NULL for defaults.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG.
 */
oef_status_t oef_set_config2d(oef_filter2d_t *filter, const oef_config_t *config);

/** @brief Reconfigure initialized 3D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @param[in] config Proposed parameters, or NULL for defaults.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG.
 */
oef_status_t oef_set_config3d(oef_filter3d_t *filter, const oef_config_t *config);

/**
 * @brief Clear scalar history but retain its configuration.
 * @param[in,out] filter Previously initialized filter.
 * @retval OEF_OK Reset, next update passes through.
 * @retval OEF_E_NULL NULL filter pointer.
 */
oef_status_t oef_reset(oef_filter_t *filter);

/** @brief Reset 2D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @return OEF_OK or OEF_E_NULL.
 */
oef_status_t oef_reset2d(oef_filter2d_t *filter);

/** @brief Reset 3D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @return OEF_OK or OEF_E_NULL.
 */
oef_status_t oef_reset3d(oef_filter3d_t *filter);

/**
 * @brief Filter one scalar sample at the supplied sampling interval.
 * @param[in,out] filter Previously initialized filter; modified only on success.
 * @param[in] input Finite raw sample in any consistent physical unit.
 * @param[in] dt_seconds Positive, finite elapsed time since the prior sample.
 * @param[out] output Receives the filtered value only on success.
 * @retval OEF_OK Success; first sample passes through unchanged.
 * @retval OEF_E_NULL NULL pointer argument.
 * @retval OEF_E_INPUT Non-finite input sample.
 * @retval OEF_E_TIME Invalid dt_seconds.
 */
oef_status_t oef_update(oef_filter_t *filter, float input,
                        float dt_seconds, float *output);

/**
 * @brief Filter one 2D vector using a shared speed-adaptive cutoff.
 * @param[in,out] filter Previously initialized 2D filter.
 * @param[in] input Finite coordinates of the same unit.
 * @param[in] dt_seconds Finite elapsed time greater than zero, in seconds.
 * @param[out] output Receives filtered coordinates only on success.
 * @return OEF_OK, OEF_E_NULL, OEF_E_INPUT or OEF_E_TIME.
 */
oef_status_t oef_update2d(oef_filter2d_t *filter, oef_vec2_t input,
                          float dt_seconds, oef_vec2_t *output);

/**
 * @brief Filter one 3D vector using a shared speed-adaptive cutoff.
 * @param[in,out] filter Previously initialized 3D filter.
 * @param[in] input Finite coordinates of the same unit.
 * @param[in] dt_seconds Finite elapsed time greater than zero, in seconds.
 * @param[out] output Receives filtered coordinates only on success.
 * @return OEF_OK, OEF_E_NULL, OEF_E_INPUT or OEF_E_TIME.
 */
oef_status_t oef_update3d(oef_filter3d_t *filter, oef_vec3_t input,
                          float dt_seconds, oef_vec3_t *output);

/**
 * @brief Clock reader callback; returns monotonic seconds.
 * @param[in] context Opaque user pointer provided when initializing.
 * @return Monotonic timestamp in seconds. Must never decrease between
 *         samples and must have sufficient resolution. Do not return
 *         wall-clock/UTC time.
 * @note A non-finite or non-increasing reading is reported as OEF_E_TIME
 *       by the next oef_auto_update*() call.
 */
typedef double (*oef_clock_fn)(void *context);

/**
 * @brief Read the built-in monotonic clock (Windows/POSIX).
 * @return Monotonic seconds. Returns 0.0 on unsupported platforms;
 *         supply an oef_clock_fn callback there instead.
 */
double oef_clock_monotonic(void);

/** @brief 1D automatic-clock filter; caller-owned and allocation-free. */
typedef struct {
    oef_filter_t filter; /**< Underlying scalar filter and configuration. */
    oef_clock_fn clock;  /**< Callback; NULL means built-in monotonic clock. */
    void *context;       /**< User pointer passed to clock callback. */
    double last_seconds; /**< Timestamp of the most recent successful update. */
} oef_auto_filter_t;

/** @brief 2D automatic-clock filter with shared vector cutoff. */
typedef struct {
    oef_filter2d_t filter; /**< Underlying vector filter and configuration. */
    oef_clock_fn clock;    /**< Callback; NULL means built-in monotonic clock. */
    void *context;         /**< User pointer passed to clock callback. */
    double last_seconds;   /**< Last committed timestamp in seconds. */
} oef_auto_filter2d_t;

/** @brief 3D automatic-clock filter with shared vector cutoff. */
typedef struct {
    oef_filter3d_t filter; /**< Underlying vector filter and configuration. */
    oef_clock_fn clock;    /**< Callback; NULL means built-in monotonic clock. */
    void *context;         /**< User pointer passed to clock callback. */
    double last_seconds;   /**< Last committed timestamp in seconds. */
} oef_auto_filter3d_t;

/**
 * @brief Initialize a 1D automatic-clock filter.
 * @param[out] filter Caller-owned writable object.
 * @param[in] config Valid filter parameters, or NULL for defaults.
 * @param[in] clock User callback or NULL for built-in monotonic clock.
 * @param[in] context Opaque pointer passed to callback; may be NULL.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG.
 */
oef_status_t oef_auto_init(oef_auto_filter_t *filter,
                          const oef_config_t *config,
                          oef_clock_fn clock, void *context);
/** @brief Initialize 2D auto-clock filter; see oef_auto_init() for contracts.
 * @param[out] filter Caller-owned writable object.
 * @param[in] config Valid parameters, or NULL for defaults.
 * @param[in] clock Callback, or NULL for built-in clock.
 * @param[in] context Callback context.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG.
 */
oef_status_t oef_auto_init2d(oef_auto_filter2d_t *filter,
                            const oef_config_t *config,
                            oef_clock_fn clock, void *context);
/** @brief Initialize 3D auto-clock filter; see oef_auto_init() for contracts.
 * @param[out] filter Caller-owned writable object.
 * @param[in] config Valid parameters, or NULL for defaults.
 * @param[in] clock Callback, or NULL for built-in clock.
 * @param[in] context Callback context.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG.
 */
oef_status_t oef_auto_init3d(oef_auto_filter3d_t *filter,
                            const oef_config_t *config,
                            oef_clock_fn clock, void *context);

/**
 * @brief Read the clock and filter a scalar, deriving dt automatically.
 * @param[in,out] filter Initialized auto-filter.
 * @param[in] input Finite scalar sample.
 * @param[out] output Written only on success.
 * @retval OEF_OK Success; first sample passes through.
 * @retval OEF_E_NULL NULL pointer argument.
 * @retval OEF_E_INPUT Non-finite input sample.
 * @retval OEF_E_TIME Clock did not advance, returned a non-finite reading,
 *         or the elapsed interval does not fit a positive float.
 * @note On error no state or output is changed. Timestamps must use one
 *       consistent monotonic time base.
 */
oef_status_t oef_auto_update(oef_auto_filter_t *filter,
                            float input, float *output);
/** @brief Auto-clock 2D update; same contracts as oef_auto_update().
 * @param[in,out] filter Initialized auto 2D filter.
 * @param[in] input Finite coordinates.
 * @param[out] output Writes filtered coordinates on success only.
 * @return OEF_OK, OEF_E_NULL, OEF_E_INPUT or OEF_E_TIME.
 */
oef_status_t oef_auto_update2d(oef_auto_filter2d_t *filter,
                              oef_vec2_t input, oef_vec2_t *output);
/** @brief Auto-clock 3D update; same contracts as oef_auto_update().
 * @param[in,out] filter Initialized auto 3D filter.
 * @param[in] input Finite coordinates.
 * @param[out] output Writes filtered coordinates on success only.
 * @return OEF_OK, OEF_E_NULL, OEF_E_INPUT or OEF_E_TIME.
 */
oef_status_t oef_auto_update3d(oef_auto_filter3d_t *filter,
                              oef_vec3_t input, oef_vec3_t *output);

/** @brief Reset 1D history and timestamp, preserving config and callback.
 * @param[in,out] filter Initialized automatic-clock scalar filter.
 * @return OEF_OK or OEF_E_NULL.
 */
oef_status_t oef_auto_reset(oef_auto_filter_t *filter);
/** @brief Reset 2D history and timestamp; see oef_auto_reset().
 * @param[in,out] filter Initialized auto 2D filter.
 * @return OEF_OK or OEF_E_NULL.
 */
oef_status_t oef_auto_reset2d(oef_auto_filter2d_t *filter);
/** @brief Reset 3D history and timestamp; see oef_auto_reset().
 * @param[in,out] filter Initialized auto 3D filter.
 * @return OEF_OK or OEF_E_NULL.
 */
oef_status_t oef_auto_reset3d(oef_auto_filter3d_t *filter);

/** @} */
#ifdef __cplusplus
}
#endif
#endif /* ONE_EURO_FILTER_H */
