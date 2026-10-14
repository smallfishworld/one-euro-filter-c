/* SPDX-License-Identifier: MIT */
#include "one_euro_filter.h"
/* Keep assertions active even when Release builds define NDEBUG. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>

/** @brief Regression test helper: near. */
static bool near(float a, float b, float epsilon)
{
    return fabsf(a - b) <= epsilon;
}

/** @brief Regression test helper: test config. */
static void test_config(void)
{
    oef_config_t c = oef_config_default();
    assert(oef_config_valid(&c));
    assert(!oef_config_valid(NULL));
    c.beta = -1.0f;
    assert(!oef_config_valid(&c));
    c = oef_config_default();
    c.max_cutoff_hz = 0.5f;
    assert(!oef_config_valid(&c));
    c = oef_config_default();
    c.min_cutoff_hz = NAN;
    assert(!oef_config_valid(&c));
    c = oef_config_default();
    c.sample_frequency_hz = 0.0f;
    assert(!oef_config_valid(&c));
}

/** @brief Regression test helper: test initial and constant. */
static void test_initial_and_constant(void)
{
    oef_filter_t f;
    oef_config_t c = oef_config_default();
    float out = -1.0f;
    unsigned int i;
    assert(oef_init(&f, &c));
    assert(oef_update(&f, 10.0f, 0.04f, &out));
    assert(out == 10.0f);
    for (i = 0; i < 100; i++) {
        assert(oef_step(&f, 10.0f, &out));
        assert(out == 10.0f);
    }
    assert(!oef_update(&f, 11.0f, 0.0f, &out));
    assert(!oef_update(&f, NAN, 0.04f, &out));
    assert(out == 10.0f && f.last_raw == 10.0f);
    oef_reset(&f);
    assert(!f.initialized);
    assert(oef_step(&f, 99.0f, &out));
    assert(out == 99.0f);
}

/** @brief Regression test helper: test smooth noise. */
static void test_smooth_noise(void)
{
    oef_filter_t f;
    oef_config_t c = oef_config_default();
    float out = 0.0f;
    float input_energy = 0.0f, filtered_energy = 0.0f;
    unsigned int i;
    c.min_cutoff_hz = 0.3f;
    c.beta = 0.0f;
    assert(oef_init(&f, &c));
    for (i = 0; i < 300; i++) {
        float raw = 10.0f + ((i % 2) ? 0.15f : -0.15f);
        assert(oef_step(&f, raw, &out));
        if (i >= 50) {
            input_energy += (raw - 10.0f) * (raw - 10.0f);
            filtered_energy += (out - 10.0f) * (out - 10.0f);
        }
    }
    assert(filtered_energy < input_energy * 0.1f);
}

/** @brief Regression test helper: test speed response. */
static void test_speed_response(void)
{
    oef_filter_t slow, fast;
    oef_config_t c = oef_config_default();
    float a, b;
    c.beta = 0.0f;
    assert(oef_init(&slow, &c));
    c.beta = 3.0f;
    assert(oef_init(&fast, &c));
    assert(oef_step(&slow, 0, &a));
    assert(oef_step(&fast, 0, &b));
    assert(oef_step(&slow, 1, &a));
    assert(oef_step(&fast, 1, &b));
    assert(b > a);
    assert(a > 0 && b < 1);
}

/** @brief Regression test helper: test reconfigure. */
static void test_reconfigure(void)
{
    oef_filter_t f;
    oef_config_t c = oef_config_default();
    float out;
    assert(oef_init(&f, &c));
    assert(oef_step(&f, 3, &out));
    c.beta = 2;
    assert(oef_set_config(&f, &c));
    assert(f.initialized);
    assert(oef_step(&f, 3, &out));
    assert(out == 3);
    c.min_cutoff_hz = -1;
    assert(!oef_set_config(&f, &c));
    assert(f.config.min_cutoff_hz > 0);
}

/** @brief Regression test helper: test dt and cutoff cap. */
static void test_dt_and_cutoff_cap(void)
{
    oef_filter_t uncapped, capped, variable;
    oef_config_t c = oef_config_default();
    float slow_out, capped_out, variable_out;
    assert(oef_init(&uncapped, &c));
    c.max_cutoff_hz = c.min_cutoff_hz;
    assert(oef_init(&capped, &c));
    assert(oef_step(&uncapped, 0.0f, &slow_out));
    assert(oef_step(&capped, 0.0f, &capped_out));
    assert(oef_step(&uncapped, 2.0f, &slow_out));
    assert(oef_step(&capped, 2.0f, &capped_out));
    assert(slow_out > capped_out);

    c = oef_config_default();
    assert(oef_init(&variable, &c));
    assert(oef_update(&variable, 0.0f, 0.02f, &variable_out));
    assert(oef_update(&variable, 5.0f, 0.02f, &variable_out));
    assert(isfinite(variable_out));
    assert(!oef_update(&variable, 5.0f, NAN, &variable_out));
    assert(!oef_update(&variable, 5.0f, INFINITY, &variable_out));
    assert(!oef_update(&variable, 5.0f, -1.0f, &variable_out));
    assert(!oef_update(NULL, 5.0f, 0.02f, &variable_out));
    assert(!oef_update(&variable, 5.0f, 0.02f, NULL));
}

/** @brief Regression test helper: test 2d and invalid. */
static void test_2d_and_invalid(void)
{
    oef_filter2d_t f;
    oef_config_t c = oef_config_default();
    oef_vec2_t out = { -9, -9 }, input = {1, 2};
    assert(oef_init2d(&f, &c));
    assert(oef_step2d(&f, input, &out));
    assert(near(out.x, 1, 0.0001f) && near(out.y, 2, 0.0001f));
    input = (oef_vec2_t){3, 4};
    assert(oef_step2d(&f, input, &out));
    /* Equal increments in X and Y use the same alpha. */
    assert(near(out.x - 1, out.y - 2, 0.0001f));
    input.x = INFINITY;
    assert(!oef_step2d(&f, input, &out));
    assert(f.last_raw.x == 3 && f.last_raw.y == 4);
    oef_reset2d(&f);
    assert(!f.initialized);
    input = (oef_vec2_t){5, 6};
    assert(oef_update2d(&f, input, 0.025f, &out));
    assert(out.x == 5 && out.y == 6);
}

/** @brief Regression test helper: test 3d and reconfigure. */
static void test_3d_and_reconfigure(void)
{
    oef_filter3d_t f;
    oef_config_t c = oef_config_default();
    oef_vec3_t raw = {1.0f, 2.0f, 3.0f};
    oef_vec3_t out = {-1.0f, -1.0f, -1.0f};
    assert(oef_init3d(&f, &c));
    assert(oef_step3d(&f, raw, &out));
    assert(out.x == raw.x && out.y == raw.y && out.z == raw.z);
    raw = (oef_vec3_t){2.0f, 3.0f, 4.0f};
    assert(oef_step3d(&f, raw, &out));
    assert(near(out.x - 1.0f, out.y - 2.0f, 1e-5f));
    assert(near(out.x - 1.0f, out.z - 3.0f, 1e-5f));
    assert(out.x > 1.0f && out.x < 2.0f);
    c.beta = 2.0f;
    assert(oef_set_config3d(&f, &c));
    assert(f.initialized && f.config.beta == 2.0f);
    raw.y = NAN;
    assert(!oef_step3d(&f, raw, &out));
    assert(f.last_raw.y == 3.0f);
    assert(!oef_update3d(&f, raw, 0.0f, &out));
    assert(!oef_update3d(&f, raw, 0.02f, NULL));
    c.min_cutoff_hz = -1.0f;
    assert(!oef_set_config3d(&f, &c));
    assert(f.config.min_cutoff_hz > 0.0f);
    oef_reset3d(&f);
    assert(!f.initialized);
    raw = (oef_vec3_t){7.0f, 8.0f, 9.0f};
    assert(oef_update3d(&f, raw, 0.01f, &out));
    assert(out.z == 9.0f);
}

/** @brief Run original API backward-compatibility test suite. */
int main(void)
{
    test_config();
    test_initial_and_constant();
    test_smooth_noise();
    test_speed_response();
    test_reconfigure();
    test_2d_and_invalid();
    test_dt_and_cutoff_cap();
    test_3d_and_reconfigure();
    puts("one-euro-filter-c: all tests passed");
    return 0;
}
