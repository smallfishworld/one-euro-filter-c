/* SPDX-License-Identifier: MIT */
/** @file test_auto_clock.c @brief Automatic sampling time and error contracts. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "one_euro_filter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

/** @brief Injectable sample clock for deterministic tests. */
typedef struct {
    double seconds; /**< Sample timestamp returned by callback. */
    int calls;      /**< Number of reads. */
} fake_clock_t;

/** @brief Callback returning a synthetic, possibly non-increasing timestamp. */
static double fake_clock(void *ctx)
{
    fake_clock_t *fake = (fake_clock_t *)ctx;
    ++fake->calls;
    return fake->seconds;
}

/** @brief Ensure exact dt calculation matches the manual update API. */
static void test_scalar_timing(void)
{
    oef_auto_filter_t automatic;
    oef_filter_t manual;
    fake_clock_t clock = {12345.0, 0};
    float auto_value = -99.0f, manual_value = -88.0f;
    oef_status_t st;
    assert(oef_auto_init(NULL, NULL, fake_clock, &clock) == OEF_E_NULL);
    /* NULL config selects the defaults. */
    assert(oef_auto_init(&automatic, NULL, fake_clock, &clock) == OEF_OK);
    assert(oef_init(&manual, NULL) == OEF_OK);

    st = oef_auto_update(&automatic, 100.0f, &auto_value);
    assert(st == OEF_OK && auto_value == 100.0f);
    assert(automatic.filter.initialized && automatic.last_seconds == 12345.0);
    assert(oef_update(&manual, 100.0f, 1.0f, &manual_value) == OEF_OK);

    clock.seconds += 0.025;
    st = oef_auto_update(&automatic, 105.0f, &auto_value);
    assert(st == OEF_OK);
    assert(oef_update(&manual, 105.0f, (float)(clock.seconds - 12345.0),
                     &manual_value) == OEF_OK);
    assert(fabsf(auto_value - manual_value) < 1.0e-6f);

    /* A failed input must not commit the clock. */
    {
        const float previous = auto_value;
        const float raw = automatic.filter.last_raw;
        const double previous_t = automatic.last_seconds;
        clock.seconds += 0.03;
        assert(oef_auto_update(&automatic, NAN, &auto_value) == OEF_E_INPUT);
        assert(auto_value == previous && automatic.filter.last_raw == raw);
        assert(automatic.last_seconds == previous_t);
        assert(oef_auto_update(&automatic, 107.0f, &auto_value) == OEF_OK);
        assert(automatic.last_seconds == clock.seconds);
    }
    assert(oef_auto_update(NULL, 10.0f, &auto_value) == OEF_E_NULL);
    assert(oef_auto_update(&automatic, 10.0f, NULL) == OEF_E_NULL);
    /* Invalid configuration is still rejected by the adapter. */
    assert(oef_auto_init(&automatic, &(oef_config_t){-1.0f, 0.5f, 1.0f},
                         fake_clock, &clock) == OEF_E_CONFIG);
}

/** @brief Non-advancing, backwards and non-finite clocks are rejected. */
static void test_clock_failures(void)
{
    oef_config_t c = oef_config_default();
    oef_auto_filter_t f;
    fake_clock_t clock = {10.0, 0};
    float out = -13.0f;
    assert(oef_auto_init(&f, &c, fake_clock, &clock) == OEF_OK);
    assert(oef_auto_update(&f, 4.0f, &out) == OEF_OK);
    out = -22.0f;
    clock.seconds = 10.0; /* Clock did not advance. */
    assert(oef_auto_update(&f, 5.0f, &out) == OEF_E_TIME);
    clock.seconds = 9.9; /* Clock went backwards. */
    assert(oef_auto_update(&f, 5.0f, &out) == OEF_E_TIME);
    clock.seconds = NAN; /* Non-finite reading. */
    assert(oef_auto_update(&f, 5.0f, &out) == OEF_E_TIME);
    assert(f.last_seconds == 10.0 && f.filter.last_raw == 4.0f);
    assert(out == -22.0f);
    assert(oef_auto_reset(&f) == OEF_OK);
    assert(!f.filter.initialized && f.last_seconds == 0.0);
    clock.seconds = 100.0;
    assert(oef_auto_update(&f, 5.0f, &out) == OEF_OK);
    assert(out == 5.0f && f.last_seconds == 100.0);
    assert(oef_auto_reset(NULL) == OEF_E_NULL);
}

/** @brief Two-dimensional callback and reset behavior. */
static void test_2d(void)
{
    oef_auto_filter2d_t f;
    fake_clock_t clock = {0.0, 0};
    oef_vec2_t out = {-99, -99};
    assert(oef_auto_init2d(&f, NULL, fake_clock, &clock) == OEF_OK);
    assert(oef_auto_update2d(&f, (oef_vec2_t){10, 20}, &out) == OEF_OK);
    assert(out.x == 10 && out.y == 20);
    clock.seconds += 0.02;
    assert(oef_auto_update2d(&f, (oef_vec2_t){12, 22}, &out) == OEF_OK);
    assert(out.x > 10 && out.x < 12);
    assert(fabsf((out.x - 10) - (out.y - 20)) < 0.0001f);
    assert(oef_auto_reset2d(&f) == OEF_OK);
    assert(!f.filter.initialized);
    assert(oef_auto_update2d(&f, (oef_vec2_t){13, 23}, &out) == OEF_OK);
    assert(out.x == 13 && out.y == 23);
    assert(oef_auto_update2d(NULL, (oef_vec2_t){10, 20}, &out) == OEF_E_NULL);
    assert(oef_auto_reset2d(NULL) == OEF_E_NULL);
    assert(oef_auto_init2d(NULL, NULL, fake_clock, &clock) == OEF_E_NULL);
}

/** @brief Three-dimensional callback and reset behavior. */
static void test_3d(void)
{
    oef_auto_filter3d_t f;
    fake_clock_t clock = {500.0, 0};
    oef_vec3_t out = {0, 0, 0};
    assert(oef_auto_init3d(&f, NULL, fake_clock, &clock) == OEF_OK);
    assert(oef_auto_update3d(&f, (oef_vec3_t){1, 2, 3}, &out) == OEF_OK);
    clock.seconds += 0.04;
    assert(oef_auto_update3d(&f, (oef_vec3_t){2, 3, 4}, &out) == OEF_OK);
    assert(out.x > 1 && out.x < 2);
    assert(oef_auto_reset3d(&f) == OEF_OK);
    assert(!f.filter.initialized);
    assert(oef_auto_update3d(&f, (oef_vec3_t){3, 4, 5}, &out) == OEF_OK);
    assert(out.z == 5);
    assert(oef_auto_reset3d(NULL) == OEF_E_NULL);
    assert(oef_auto_init3d(NULL, NULL, fake_clock, &clock) == OEF_E_NULL);
}

/** @brief Built-in monotonic clock resolves without caller-supplied timestamps. */
static void test_platform_clock(void)
{
    double first = oef_clock_monotonic();
    double next = oef_clock_monotonic();
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    assert(isfinite(first) && isfinite(next) && next >= first);
#else
    assert(first == 0.0 && next == 0.0);
#endif
}

/** @brief Automatic timing test entry point. */
int main(void)
{
    test_scalar_timing();
    test_clock_failures();
    test_2d();
    test_3d();
    test_platform_clock();
    puts("one-euro-filter-c: automatic clock tests passed");
    return 0;
}
