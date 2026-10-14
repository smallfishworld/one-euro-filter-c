/* SPDX-License-Identifier: MIT */
#include "one_euro_filter.h"
#include <math.h>
#include <string.h>

#define OEF_TWO_PI 6.2831853071795864769f

static bool valid2(oef_vec2_t v)
{
    return isfinite(v.x) && isfinite(v.y);
}

static bool valid3(oef_vec3_t v)
{
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

static float alpha_for(float cutoff, float dt)
{
    /* Algebraically equivalent to dt/(dt + 1/(2*pi*cutoff)). */
    float z = OEF_TWO_PI * cutoff * dt;
    return z / (1.0f + z);
}

static float cutoff_for(const oef_config_t *c, float speed)
{
    float cutoff = c->min_cutoff_hz + c->beta * speed;
    if (c->max_cutoff_hz > 0.0f && cutoff > c->max_cutoff_hz) {
        cutoff = c->max_cutoff_hz;
    }
    return cutoff;
}

oef_config_t oef_config_default(void)
{
    oef_config_t cfg = {0.8f, 0.5f, 1.0f, 25.0f, 0.0f};
    return cfg;
}

bool oef_config_valid(const oef_config_t *c)
{
    return c != NULL &&
           isfinite(c->min_cutoff_hz) && c->min_cutoff_hz > 0.0f &&
           isfinite(c->beta) && c->beta >= 0.0f &&
           isfinite(c->derivative_cutoff_hz) &&
           c->derivative_cutoff_hz > 0.0f &&
           isfinite(c->sample_frequency_hz) &&
           c->sample_frequency_hz > 0.0f &&
           isfinite(c->max_cutoff_hz) &&
           (c->max_cutoff_hz == 0.0f ||
            c->max_cutoff_hz >= c->min_cutoff_hz);
}

bool oef_init(oef_filter_t *f, const oef_config_t *c)
{
    if (!f || !oef_config_valid(c)) return false;
    memset(f, 0, sizeof(*f));
    f->config = *c;
    return true;
}
bool oef_init2d(oef_filter2d_t *f, const oef_config_t *c)
{
    if (!f || !oef_config_valid(c)) return false;
    memset(f, 0, sizeof(*f));
    f->config = *c;
    return true;
}
bool oef_init3d(oef_filter3d_t *f, const oef_config_t *c)
{
    if (!f || !oef_config_valid(c)) return false;
    memset(f, 0, sizeof(*f));
    f->config = *c;
    return true;
}

bool oef_set_config(oef_filter_t *f, const oef_config_t *c)
{
    if (!f || !oef_config_valid(c)) return false;
    f->config = *c;
    return true;
}
bool oef_set_config2d(oef_filter2d_t *f, const oef_config_t *c)
{
    if (!f || !oef_config_valid(c)) return false;
    f->config = *c;
    return true;
}
bool oef_set_config3d(oef_filter3d_t *f, const oef_config_t *c)
{
    if (!f || !oef_config_valid(c)) return false;
    f->config = *c;
    return true;
}

void oef_reset(oef_filter_t *f)
{
    if (!f) return;
    f->last_raw = f->last_filtered = f->last_velocity = 0.0f;
    f->initialized = false;
}
void oef_reset2d(oef_filter2d_t *f)
{
    if (!f) return;
    f->last_raw = (oef_vec2_t){0};
    f->last_filtered = (oef_vec2_t){0};
    f->last_velocity = (oef_vec2_t){0};
    f->initialized = false;
}
void oef_reset3d(oef_filter3d_t *f)
{
    if (!f) return;
    f->last_raw = (oef_vec3_t){0};
    f->last_filtered = (oef_vec3_t){0};
    f->last_velocity = (oef_vec3_t){0};
    f->initialized = false;
}

bool oef_update(oef_filter_t *f, float x, float dt, float *output)
{
    float v, vd, a_d, fc, a, y;
    if (!f || !output || !oef_config_valid(&f->config) ||
        !isfinite(x) || !isfinite(dt) || dt <= 0.0f)
        return false;
    if (!f->initialized) {
        f->last_raw = f->last_filtered = x;
        f->last_velocity = 0.0f;
        f->initialized = true;
        *output = x;
        return true;
    }
    v = (x - f->last_raw) / dt;
    a_d = alpha_for(f->config.derivative_cutoff_hz, dt);
    if (!isfinite(v) || !isfinite(a_d)) return false;
    vd = f->last_velocity + a_d * (v - f->last_velocity);
    fc = cutoff_for(&f->config, fabsf(vd));
    a = alpha_for(fc, dt);
    if (!isfinite(vd) || !isfinite(fc) || !isfinite(a)) return false;
    y = f->last_filtered + a * (x - f->last_filtered);
    if (!isfinite(y)) return false;
    f->last_raw = x;
    f->last_velocity = vd;
    f->last_filtered = y;
    *output = y;
    return true;
}

bool oef_update2d(oef_filter2d_t *f, oef_vec2_t x, float dt,
                  oef_vec2_t *output)
{
    oef_vec2_t v, vd, y;
    float a_d, speed, fc, a;
    if (!f || !output || !oef_config_valid(&f->config) ||
        !valid2(x) || !isfinite(dt) || dt <= 0.0f)
        return false;
    if (!f->initialized) {
        f->last_raw = f->last_filtered = x;
        f->last_velocity = (oef_vec2_t){0};
        f->initialized = true;
        *output = x;
        return true;
    }
    v.x = (x.x - f->last_raw.x) / dt;
    v.y = (x.y - f->last_raw.y) / dt;
    a_d = alpha_for(f->config.derivative_cutoff_hz, dt);
    if (!valid2(v) || !isfinite(a_d)) return false;
    vd.x = f->last_velocity.x + a_d * (v.x - f->last_velocity.x);
    vd.y = f->last_velocity.y + a_d * (v.y - f->last_velocity.y);
    speed = hypotf(vd.x, vd.y);
    fc = cutoff_for(&f->config, speed);
    a = alpha_for(fc, dt);
    if (!valid2(vd) || !isfinite(speed) || !isfinite(fc) || !isfinite(a))
        return false;
    /* One alpha for both axes reduces directional distortion. */
    y.x = f->last_filtered.x + a * (x.x - f->last_filtered.x);
    y.y = f->last_filtered.y + a * (x.y - f->last_filtered.y);
    if (!valid2(y)) return false;
    f->last_raw = x;
    f->last_velocity = vd;
    f->last_filtered = y;
    *output = y;
    return true;
}

bool oef_update3d(oef_filter3d_t *f, oef_vec3_t x, float dt,
                  oef_vec3_t *output)
{
    oef_vec3_t v, vd, y;
    float a_d, speed, fc, a;
    if (!f || !output || !oef_config_valid(&f->config) ||
        !valid3(x) || !isfinite(dt) || dt <= 0.0f)
        return false;
    if (!f->initialized) {
        f->last_raw = f->last_filtered = x;
        f->last_velocity = (oef_vec3_t){0};
        f->initialized = true;
        *output = x;
        return true;
    }
    v.x = (x.x - f->last_raw.x) / dt;
    v.y = (x.y - f->last_raw.y) / dt;
    v.z = (x.z - f->last_raw.z) / dt;
    a_d = alpha_for(f->config.derivative_cutoff_hz, dt);
    if (!valid3(v) || !isfinite(a_d)) return false;
    vd.x = f->last_velocity.x + a_d * (v.x - f->last_velocity.x);
    vd.y = f->last_velocity.y + a_d * (v.y - f->last_velocity.y);
    vd.z = f->last_velocity.z + a_d * (v.z - f->last_velocity.z);
    speed = hypotf(hypotf(vd.x, vd.y), vd.z);
    fc = cutoff_for(&f->config, speed);
    a = alpha_for(fc, dt);
    if (!valid3(vd) || !isfinite(speed) || !isfinite(fc) || !isfinite(a))
        return false;
    y.x = f->last_filtered.x + a * (x.x - f->last_filtered.x);
    y.y = f->last_filtered.y + a * (x.y - f->last_filtered.y);
    y.z = f->last_filtered.z + a * (x.z - f->last_filtered.z);
    if (!valid3(y)) return false;
    f->last_raw = x;
    f->last_velocity = vd;
    f->last_filtered = y;
    *output = y;
    return true;
}

bool oef_step(oef_filter_t *f, float x, float *out)
{
    if (!f || !oef_config_valid(&f->config)) return false;
    return oef_update(f, x, 1.0f / f->config.sample_frequency_hz, out);
}
bool oef_step2d(oef_filter2d_t *f, oef_vec2_t x, oef_vec2_t *out)
{
    if (!f || !oef_config_valid(&f->config)) return false;
    return oef_update2d(f, x, 1.0f / f->config.sample_frequency_hz, out);
}
bool oef_step3d(oef_filter3d_t *f, oef_vec3_t x, oef_vec3_t *out)
{
    if (!f || !oef_config_valid(&f->config)) return false;
    return oef_update3d(f, x, 1.0f / f->config.sample_frequency_hz, out);
}
