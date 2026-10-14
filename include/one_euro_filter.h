/* SPDX-License-Identifier: MIT */
/**
 * @file one_euro_filter.h
 * @brief Allocation-free, configurable 1D/2D/3D One Euro Filter (C99).
 *
 * @details All cutoff frequencies are in Hz, time intervals in seconds.
 * A filter instance must be initialized before any operation other than init.
 * Concurrent access to the SAME instance requires caller synchronization.
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
#define OEF_VERSION_MAJOR 1
/** @brief Semantic-version minor number. */
#define OEF_VERSION_MINOR 1
/** @brief Semantic-version patch number. */
#define OEF_VERSION_PATCH 0

/**
 * @brief Cross-platform operation result.
 * @details Success is zero; errors are distinct, stable negative values.
 * These values are library-defined and MUST NOT be treated as POSIX errno.
 */
typedef enum {
    OEF_OK        =  0, /**< Operation completed successfully. */
    OEF_E_NULL    = -1, /**< Required pointer argument is NULL. */
    OEF_E_CONFIG  = -2, /**< Requested configuration contains invalid values. */
    OEF_E_INPUT   = -3, /**< Input sample contains NaN or infinity. */
    OEF_E_TIME    = -4, /**< Sample interval is zero, negative or non-finite. */
    OEF_E_STATE   = -5, /**< Filter not initialized or stored state is invalid. */
    OEF_E_RANGE   = -6  /**< An intermediate or stored value exceeds float range. */
} oef_status_t;

/**
 * @brief Tunable parameters for one filter instance.
 * @details All cutoff frequencies are positive and finite. Speed is measured
 * in input units per second; beta therefore depends on input units.
 * Use oef_config_default() as the recommended initializer.
 */
typedef struct {
    float min_cutoff_hz;        /**< Minimum low-pass cutoff, >0 Hz. */
    float beta;                 /**< Velocity sensitivity, finite and >=0. */
    float derivative_cutoff_hz; /**< Velocity low-pass cutoff, >0 Hz. */
    float sample_frequency_hz; /**< Nominal frequency for oef_step*(), >0 Hz. */
    float max_cutoff_hz;        /**< 0 disables cap; otherwise >= min_cutoff_hz. */
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
 * @warning Treat fields other than config as read-only. Do not manually edit
 * history or copy a live object while it is being updated.
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
    oef_config_t config;        /**< Current validated configuration. */
    oef_vec2_t last_raw;        /**< Previous unfiltered vector. */
    oef_vec2_t last_filtered;   /**< Previous filtered vector. */
    oef_vec2_t last_velocity;   /**< Previous filtered velocity (units/s). */
    bool initialized;           /**< True after the first successful vector. */
} oef_filter2d_t;

/** @brief Stateful 3D vector filter using one shared adaptive cutoff. */
typedef struct {
    oef_config_t config;        /**< Current validated configuration. */
    oef_vec3_t last_raw;        /**< Previous unfiltered vector. */
    oef_vec3_t last_filtered;   /**< Previous filtered vector. */
    oef_vec3_t last_velocity;   /**< Previous filtered velocity (units/s). */
    bool initialized;           /**< True after the first successful vector. */
} oef_filter3d_t;

/**
 * @brief Obtain initial parameter values.
 * @return Config (0.8 Hz, 0.5, 1.0 Hz, 25 Hz, no cutoff cap).
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
 * @param[out] filter Caller-owned instance, writable and non-NULL.
 * @param[in] config Fully initialized parameter struct, non-NULL.
 * @retval OEF_OK Initialized; first update will return input unchanged.
 * @retval OEF_E_NULL One or more pointers are NULL.
 * @retval OEF_E_CONFIG Invalid configuration; filter is not modified.
 */
oef_status_t oef_init_ex(oef_filter_t *filter, const oef_config_t *config);

/** @brief Initialize 2D filter; see oef_init_ex() for status and contracts.
 * @param[out] filter Initialized caller-owned filter.
 * @param[in] config Valid parameters, non-NULL.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG; failure preserves filter.
 */
oef_status_t oef_init2d_ex(oef_filter2d_t *filter, const oef_config_t *config);
/** @brief Initialize 3D filter; see oef_init_ex() for status and contracts.
 * @param[out] filter Initialized caller-owned filter.
 * @param[in] config Valid parameters, non-NULL.
 * @return OEF_OK, OEF_E_NULL or OEF_E_CONFIG; failure preserves filter.
 */
oef_status_t oef_init3d_ex(oef_filter3d_t *filter, const oef_config_t *config);

/**
 * @brief Change scalar configuration while retaining filter history.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] config New valid parameter set.
 * @retval OEF_OK Configuration applied.
 * @retval OEF_E_NULL NULL argument.
 * @retval OEF_E_CONFIG New configuration is invalid (no mutation).
 * @retval OEF_E_STATE Current filter configuration/state is invalid.
 * @note Call oef_reset_ex() if a physical unit or tracked signal changes.
 */
oef_status_t oef_set_config_ex(oef_filter_t *filter, const oef_config_t *config);
/** @brief Reconfigure initialized 2D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @param[in] config Proposed valid parameters.
 * @return OEF_OK, OEF_E_NULL, OEF_E_CONFIG or OEF_E_STATE.
 */
oef_status_t oef_set_config2d_ex(oef_filter2d_t *filter, const oef_config_t *config);
/** @brief Reconfigure initialized 3D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @param[in] config Proposed valid parameters.
 * @return OEF_OK, OEF_E_NULL, OEF_E_CONFIG or OEF_E_STATE.
 */
oef_status_t oef_set_config3d_ex(oef_filter3d_t *filter, const oef_config_t *config);

/**
 * @brief Clear scalar history but retain its configuration.
 * @param[in,out] filter Previously initialized filter.
 * @retval OEF_OK Reset, next update passes through.
 * @retval OEF_E_NULL NULL filter pointer.
 * @retval OEF_E_STATE Filter's configuration is invalid.
 */
oef_status_t oef_reset_ex(oef_filter_t *filter);
/** @brief Reset 2D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @return OEF_OK, OEF_E_NULL or OEF_E_STATE.
 */
oef_status_t oef_reset2d_ex(oef_filter2d_t *filter);
/** @brief Reset 3D filter; same contracts as scalar.
 * @param[in,out] filter Previously initialized instance.
 * @return OEF_OK, OEF_E_NULL or OEF_E_STATE.
 */
oef_status_t oef_reset3d_ex(oef_filter3d_t *filter);

/**
 * @brief Filter one scalar sample at the supplied real sampling interval.
 * @param[in,out] filter Previously initialized filter; modified only on success.
 * @param[in] input Finite raw sample in any consistent physical unit.
 * @param[in] dt_seconds Positive, finite elapsed time since prior sample.
 * @param[out] output Receives value only on success; must not alias state.
 * @retval OEF_OK Success; first sample is passed through unchanged.
 * @retval OEF_E_NULL NULL pointer argument.
 * @retval OEF_E_STATE Invalid current configuration or state.
 * @retval OEF_E_INPUT Non-finite input sample.
 * @retval OEF_E_TIME Invalid dt_seconds.
 * @retval OEF_E_RANGE Calculation cannot fit into stored float state.
 * @note First use still requires valid dt; failure is transactional.
 */
oef_status_t oef_update_ex(oef_filter_t *filter, float input,
                           float dt_seconds, float *output);

/**
 * @brief Filter one 2D vector using shared speed-adaptive cutoff.
 * @param[in,out] filter Previously initialized 2D filter.
 * @param[in] input Finite coordinates of the same unit.
 * @param[in] dt_seconds Finite elapsed time greater than zero, in seconds.
 * @param[out] output Receives filtered coordinates only on success.
 * @return OEF_OK or one of OEF_E_NULL, OEF_E_STATE, OEF_E_INPUT,
 * OEF_E_TIME, OEF_E_RANGE (see oef_update_ex()).
 */
oef_status_t oef_update2d_ex(oef_filter2d_t *filter, oef_vec2_t input,
                             float dt_seconds, oef_vec2_t *output);

/**
 * @brief Filter one 3D vector using shared speed-adaptive cutoff.
 * @param[in,out] filter Previously initialized 3D filter.
 * @param[in] input Finite coordinates of the same unit.
 * @param[in] dt_seconds Finite elapsed time greater than zero, in seconds.
 * @param[out] output Receives filtered coordinates only on success.
 * @return OEF_OK or one of OEF_E_NULL, OEF_E_STATE, OEF_E_INPUT,
 * OEF_E_TIME, OEF_E_RANGE (see oef_update_ex()).
 */
oef_status_t oef_update3d_ex(oef_filter3d_t *filter, oef_vec3_t input,
                             float dt_seconds, oef_vec3_t *output);

/**
 * @brief Filter one scalar sample at configured sample_frequency_hz.
 * @param[in,out] filter Previously initialized scalar filter.
 * @param[in] input Finite raw sample.
 * @param[out] output Value written on success only.
 * @return OEF_OK or same error codes as oef_update_ex().
 * @note Return OEF_E_RANGE if 1/frequency cannot be represented as float.
 */
oef_status_t oef_step_ex(oef_filter_t *filter, float input, float *output);
/**
 * @brief Filter 2D sample using configured sampling rate.
 * @param[in,out] filter Previously initialized 2D filter.
 * @param[in] input Finite vector sample.
 * @param[out] output Filtered vector on success.
 * @return OEF_OK or one of the errors documented by oef_step_ex().
 */
oef_status_t oef_step2d_ex(oef_filter2d_t *filter, oef_vec2_t input,
                           oef_vec2_t *output);
/**
 * @brief Filter 3D sample using configured sampling rate.
 * @param[in,out] filter Previously initialized 3D filter.
 * @param[in] input Finite vector sample.
 * @param[out] output Filtered vector on success.
 * @return OEF_OK or one of the errors documented by oef_step_ex().
 */
oef_status_t oef_step3d_ex(oef_filter3d_t *filter, oef_vec3_t input,
                           oef_vec3_t *output);

/* Backward-compatible 1.0 convenience wrappers. They return true only for
 * OEF_OK; use the *_ex functions above for exact error details. */

/** @brief Legacy scalar initializer.
 * @return true if oef_init_ex() succeeds.
 * @param[out] filter Caller-owned writable instance.
 * @param[in] config New valid configuration.
 * @return true iff the status-returning initializer returns OEF_OK.
 */
bool oef_init(oef_filter_t *filter, const oef_config_t *config);
/** @brief Legacy 2D initializer.
 * @return true if oef_init2d_ex() succeeds.
 * @param[out] filter Caller-owned writable instance.
 * @param[in] config New valid configuration.
 * @return true iff the status-returning initializer returns OEF_OK.
 */
bool oef_init2d(oef_filter2d_t *filter, const oef_config_t *config);
/** @brief Legacy 3D initializer.
 * @return true if oef_init3d_ex() succeeds.
 * @param[out] filter Caller-owned writable instance.
 * @param[in] config New valid configuration.
 * @return true iff the status-returning initializer returns OEF_OK.
 */
bool oef_init3d(oef_filter3d_t *filter, const oef_config_t *config);
/** @brief Legacy scalar reconfiguration.
 * @return true on success.
 * @param[in,out] filter Previously initialized instance.
 * @param[in] config Proposed configuration.
 * @return true iff the status-returning reconfiguration returns OEF_OK.
 */
bool oef_set_config(oef_filter_t *filter, const oef_config_t *config);
/** @brief Legacy 2D reconfiguration.
 * @return true on success.
 * @param[in,out] filter Previously initialized instance.
 * @param[in] config Proposed configuration.
 * @return true iff the status-returning reconfiguration returns OEF_OK.
 */
bool oef_set_config2d(oef_filter2d_t *filter, const oef_config_t *config);
/** @brief Legacy 3D reconfiguration.
 * @return true on success.
 * @param[in,out] filter Previously initialized instance.
 * @param[in] config Proposed configuration.
 * @return true iff the status-returning reconfiguration returns OEF_OK.
 */
bool oef_set_config3d(oef_filter3d_t *filter, const oef_config_t *config);
/** @brief Legacy scalar reset; discards status.
 * @param[in,out] filter Previously initialized instance; NULL is ignored.
 * @note Failure is discarded. Use the *_ex equivalent for errors.
 */
void oef_reset(oef_filter_t *filter);
/** @brief Legacy 2D reset; discards status.
 * @param[in,out] filter Previously initialized instance; NULL is ignored.
 * @note Failure is discarded. Use the *_ex equivalent for errors.
 */
void oef_reset2d(oef_filter2d_t *filter);
/** @brief Legacy 3D reset; discards status.
 * @param[in,out] filter Previously initialized instance; NULL is ignored.
 * @note Failure is discarded. Use the *_ex equivalent for errors.
 */
void oef_reset3d(oef_filter3d_t *filter);
/** @brief Legacy scalar update.
 * @return true on success.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] input Finite scalar or vector input.
 * @param[in] dt_seconds Positive finite elapsed time, seconds.
 * @param[out] output Written only on success.
 * @return true iff the status-returning update returns OEF_OK.
 */
bool oef_update(oef_filter_t *filter, float input, float dt_seconds,
                float *output);
/** @brief Legacy 2D update.
 * @return true on success.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] input Finite scalar or vector input.
 * @param[in] dt_seconds Positive finite elapsed time, seconds.
 * @param[out] output Written only on success.
 * @return true iff the status-returning update returns OEF_OK.
 */
bool oef_update2d(oef_filter2d_t *filter, oef_vec2_t input, float dt_seconds,
                  oef_vec2_t *output);
/** @brief Legacy 3D update.
 * @return true on success.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] input Finite scalar or vector input.
 * @param[in] dt_seconds Positive finite elapsed time, seconds.
 * @param[out] output Written only on success.
 * @return true iff the status-returning update returns OEF_OK.
 */
bool oef_update3d(oef_filter3d_t *filter, oef_vec3_t input, float dt_seconds,
                  oef_vec3_t *output);
/** @brief Legacy scalar fixed-rate update.
 * @return true on success.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] input Finite scalar or vector input.
 * @param[out] output Written only on success.
 * @return true iff the status-returning step returns OEF_OK.
 */
bool oef_step(oef_filter_t *filter, float input, float *output);
/** @brief Legacy 2D fixed-rate update.
 * @return true on success.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] input Finite scalar or vector input.
 * @param[out] output Written only on success.
 * @return true iff the status-returning step returns OEF_OK.
 */
bool oef_step2d(oef_filter2d_t *filter, oef_vec2_t input, oef_vec2_t *output);
/** @brief Legacy 3D fixed-rate update.
 * @return true on success.
 * @param[in,out] filter Previously initialized filter.
 * @param[in] input Finite scalar or vector input.
 * @param[out] output Written only on success.
 * @return true iff the status-returning step returns OEF_OK.
 */
bool oef_step3d(oef_filter3d_t *filter, oef_vec3_t input, oef_vec3_t *output);

/** @} */
#ifdef __cplusplus
}
#endif
#endif /* ONE_EURO_FILTER_H */
