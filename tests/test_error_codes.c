/* SPDX-License-Identifier: MIT */
/** @file test_error_codes.c @brief API contract, failure atomicity and edge cases. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "one_euro_filter.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/** @brief Verify exact symbolic and readable status mappings. */
static void test_status_names(void)
{
    assert(strcmp(oef_status_name(OEF_OK), "OEF_OK") == 0);
    assert(strcmp(oef_status_name(OEF_E_NULL), "OEF_E_NULL") == 0);
    assert(strcmp(oef_status_name(OEF_E_CONFIG), "OEF_E_CONFIG") == 0);
    assert(strcmp(oef_status_name(OEF_E_INPUT), "OEF_E_INPUT") == 0);
    assert(strcmp(oef_status_name(OEF_E_TIME), "OEF_E_TIME") == 0);
    assert(strcmp(oef_status_name(OEF_E_STATE), "OEF_E_STATE") == 0);
    assert(strcmp(oef_status_name(OEF_E_RANGE), "OEF_E_RANGE") == 0);
    assert(strcmp(oef_status_name(OEF_E_CLOCK), "OEF_E_CLOCK") == 0);
    assert(strcmp(oef_status_name((oef_status_t)-999), "OEF_UNKNOWN") == 0);
    assert(oef_status_message(OEF_OK)[0] != '\0');
    assert(oef_status_message(OEF_E_RANGE)[0] != '\0');
    assert(oef_status_message((oef_status_t)-999)[0] != '\0');
}

/** @brief Invalid initialization must not damage caller-owned instances. */
static void test_initialize_transactional(void)
{
    oef_filter_t scalar;
    oef_filter2d_t xy;
    oef_filter3d_t xyz;
    oef_config_t valid = oef_config_default();
    oef_config_t invalid = valid;
    memset(&scalar, 0xA5, sizeof(scalar));
    memset(&xy, 0xA5, sizeof(xy));
    memset(&xyz, 0xA5, sizeof(xyz));
    assert(oef_init_ex(NULL, &valid) == OEF_E_NULL);
    assert(oef_init_ex(&scalar, NULL) == OEF_E_NULL);
    invalid.min_cutoff_hz = NAN;
    assert(oef_init_ex(&scalar, &invalid) == OEF_E_CONFIG);
    assert(oef_init2d_ex(&xy, &invalid) == OEF_E_CONFIG);
    assert(oef_init3d_ex(&xyz, &invalid) == OEF_E_CONFIG);
    assert(oef_init2d_ex(NULL, &valid) == OEF_E_NULL);
    assert(oef_init3d_ex(NULL, &valid) == OEF_E_NULL);
    /* No mutation on failure, tested using known patterns without reading bool. */
    {
        unsigned char pattern[sizeof(scalar)];
        memset(pattern, 0xA5, sizeof(pattern));
        assert(memcmp(&scalar, pattern, sizeof(pattern)) == 0);
    }
    assert(oef_init_ex(&scalar, &valid) == OEF_OK);
    /* Aliasing configuration with filter storage must work during re-init. */
    assert(oef_init_ex(&scalar, &scalar.config) == OEF_OK);
    assert(oef_init2d_ex(&xy, &valid) == OEF_OK);
    assert(oef_init3d_ex(&xyz, &valid) == OEF_OK);
}

/** @brief Check each individual configuration field and rollback. */
static void test_config_validation(void)
{
    oef_config_t c = oef_config_default();
    oef_filter_t f;
    float out;
    assert(oef_config_valid(&c));
    assert(!oef_config_valid(NULL));
    c.min_cutoff_hz = 0.0f;
    assert(!oef_config_valid(&c));
    c = oef_config_default(); c.beta = -1.0f;
    assert(!oef_config_valid(&c));
    c = oef_config_default(); c.derivative_cutoff_hz = INFINITY;
    assert(!oef_config_valid(&c));
    c = oef_config_default(); c.max_cutoff_hz = -0.1f;
    assert(!oef_config_valid(&c));
    c = oef_config_default(); c.max_cutoff_hz = 0.2f;
    assert(!oef_config_valid(&c));
    c = oef_config_default();
    assert(oef_init_ex(&f, &c) == OEF_OK);
    assert(oef_update_ex(&f, 10.0f, 0.04f, &out) == OEF_OK);
    c.beta = -1.0f;
    assert(oef_set_config_ex(&f, &c) == OEF_E_CONFIG);
    assert(f.config.beta == 0.5f && f.initialized);
    assert(oef_set_config_ex(NULL, &c) == OEF_E_NULL);
    assert(oef_set_config_ex(&f, NULL) == OEF_E_NULL);
}

/** @brief Invalid dt/input do not change output or internal state. */
static void test_scalar_error_atomicity(void)
{
    oef_filter_t f;
    oef_config_t c = oef_config_default();
    float out = 123.0f;
    float old_raw, old_filtered, old_velocity;
    assert(oef_init_ex(&f, &c) == OEF_OK);
    assert(oef_update_ex(&f, 9.0f, 0.02f, &out) == OEF_OK);
    assert(out == 9.0f);
    old_raw = f.last_raw;
    old_filtered = f.last_filtered;
    old_velocity = f.last_velocity;
    out = -100.0f;
    assert(oef_update_ex(&f, 10.0f, 0.0f, &out) == OEF_E_TIME);
    assert(oef_update_ex(&f, 10.0f, -0.01f, &out) == OEF_E_TIME);
    assert(oef_update_ex(&f, 10.0f, NAN, &out) == OEF_E_TIME);
    assert(oef_update_ex(&f, 10.0f, INFINITY, &out) == OEF_E_TIME);
    assert(oef_update_ex(&f, NAN, 0.02f, &out) == OEF_E_INPUT);
    assert(oef_update_ex(&f, INFINITY, 0.02f, &out) == OEF_E_INPUT);
    assert(oef_update_ex(&f, 10.0f, 0.02f, NULL) == OEF_E_NULL);
    assert(oef_update_ex(NULL, 10.0f, 0.02f, &out) == OEF_E_NULL);
    assert(out == -100.0f);
    assert(f.last_raw == old_raw && f.last_filtered == old_filtered &&
           f.last_velocity == old_velocity && f.initialized);
    assert(oef_update_ex(&f, -FLT_MAX, FLT_MIN, &out) == OEF_E_RANGE);
    assert(f.last_raw == old_raw && f.last_velocity == old_velocity);
    assert(out == -100.0f);
    assert(oef_update_ex(&f, 9.01f, 0.02f, &out) == OEF_OK);
    assert(isfinite(out));
    f.last_velocity = NAN;
    out = 55.0f;
    assert(oef_update_ex(&f, 9.0f, 0.02f, &out) == OEF_E_STATE);
    assert(out == 55.0f);
    assert(oef_reset_ex(&f) == OEF_OK);
    assert(oef_update_ex(&f, 5.0f, 0.02f, &out) == OEF_OK);
    assert(out == 5.0f);
}

/** @brief Non-initialized, invalid config and manual dt edge cases. */
static void test_lifecycle_and_extreme_rate(void)
{
    oef_filter_t filter = {0};
    oef_config_t c = oef_config_default();
    float out = -200.0f;
    assert(oef_update_ex(&filter, 12.0f, 0.04f, &out) == OEF_E_STATE);
    assert(oef_reset_ex(&filter) == OEF_E_STATE);
    assert(oef_init_ex(&filter, &c) == OEF_OK);
    assert(oef_reset_ex(&filter) == OEF_OK);
    assert(!filter.initialized);
    assert(oef_update_ex(&filter, 12.0f, 0.04f, &out) == OEF_OK);
    assert(out == 12.0f);
    filter.config.min_cutoff_hz = NAN;
    assert(oef_reset_ex(&filter) == OEF_E_STATE);
    assert(oef_update_ex(&filter, 15.0f, 0.04f, &out) == OEF_E_STATE);
    assert(out == 12.0f);
    assert(oef_init_ex(&filter, &c) == OEF_OK);
}

/** @brief All vector interfaces reject bad input and preserve prior data. */
static void test_vector_error_atomicity(void)
{
    oef_filter2d_t xy;
    oef_filter3d_t xyz;
    oef_config_t c = oef_config_default();
    oef_vec2_t a = {3.0f, 4.0f}, out2 = {-100.0f, -101.0f};
    oef_vec3_t b = {3.0f, 4.0f, 5.0f}, out3 = {-100.0f, -101.0f, -102.0f};
    assert(oef_init2d_ex(&xy, &c) == OEF_OK);
    assert(oef_init3d_ex(&xyz, &c) == OEF_OK);
    assert(oef_update2d_ex(&xy, a, 0.04f, &out2) == OEF_OK);
    assert(oef_update3d_ex(&xyz, b, 0.04f, &out3) == OEF_OK);
    a.x = NAN; b.z = INFINITY;
    out2.x = out2.y = 101.0f;
    out3.x = out3.y = out3.z = 201.0f;
    assert(oef_update2d_ex(&xy, a, 0.04f, &out2) == OEF_E_INPUT);
    assert(oef_update3d_ex(&xyz, b, 0.04f, &out3) == OEF_E_INPUT);
    assert(oef_update2d_ex(&xy, a, 0.0f, &out2) == OEF_E_INPUT);
    assert(oef_update3d_ex(&xyz, b, 0.0f, &out3) == OEF_E_INPUT);
    assert(oef_update2d_ex(&xy, a, 0.02f, NULL) == OEF_E_NULL);
    assert(oef_update3d_ex(&xyz, b, 0.02f, NULL) == OEF_E_NULL);
    assert(out2.x == 101.0f && out2.y == 101.0f);
    assert(out3.x == 201.0f && out3.y == 201.0f && out3.z == 201.0f);
    assert(xy.last_raw.x == 3.0f && xyz.last_raw.z == 5.0f);
    c.beta = 2.0f;
    assert(oef_set_config2d_ex(&xy, &c) == OEF_OK);
    assert(oef_set_config3d_ex(&xyz, &c) == OEF_OK);
    c.beta = -1.0f;
    assert(oef_set_config2d_ex(&xy, &c) == OEF_E_CONFIG);
    assert(oef_set_config3d_ex(&xyz, &c) == OEF_E_CONFIG);
    assert(xy.config.beta == 2.0f && xyz.config.beta == 2.0f);
    assert(oef_reset2d_ex(&xy) == OEF_OK);
    assert(oef_reset3d_ex(&xyz) == OEF_OK);
    assert(!xy.initialized && !xyz.initialized);
}

/** @brief Deterministic pseudorandom stress: boundedness and finite output. */
static void test_stress(void)
{
    oef_filter_t f;
    oef_config_t c = oef_config_default();
    unsigned int seed = 123456789U;
    float last = 0.0f;
    unsigned int i;
    assert(oef_init_ex(&f, &c) == OEF_OK);
    for (i = 0U; i < 25000U; ++i) {
        float input, output;
        float min_value, max_value;
        seed = seed * 1664525U + 1013904223U;
        input = (float)(seed % 2001U) * 0.01f - 10.0f;
        min_value = (input < last) ? input : last;
        max_value = (input > last) ? input : last;
        assert(oef_update_ex(&f, input, (float)(1U + i % 30U) * 0.002f,
                             &output) == OEF_OK);
        assert(isfinite(output));
        if (i != 0U) {
            assert(output >= min_value - 1e-5f);
            assert(output <= max_value + 1e-5f);
        }
        last = output;
    }
}

/** @brief Entry point for API contract checks. */
int main(void)
{
    test_status_names();
    test_initialize_transactional();
    test_config_validation();
    test_scalar_error_atomicity();
    test_lifecycle_and_extreme_rate();
    test_vector_error_atomicity();
    test_stress();
    puts("one-euro-filter-c: error-code, edge-case and stress tests passed");
    return 0;
}
