# one-euro-filter-c

[![C CI](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml/badge.svg)](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

**English** | [简体中文](README.zh-CN.md)

A **small, allocation-free C99 library** implementing the *One Euro Filter* for 1D values, 2D points, and 3D positions. Smooth stationary measurement jitter while responding more quickly to meaningful movement. Suitable for sensors, UI pointers and tracking data.

Version **3.0.0** simplifies the public API to a single status-returning set of functions, allows `NULL` configurations, and reduces the clock interface to a one-line callback.

## Features

- **Automatic timing:** `oef_auto_update*()` reads a monotonic clock and derives elapsed seconds between *successfully processed* samples.
- **Explicit timing:** `oef_update*(..., dt_seconds, ...)` for externally timestamped measurements or deterministic processing.
- **Platform clock:** Linux/macOS use `clock_gettime(CLOCK_MONOTONIC)`; Windows uses `QueryPerformanceCounter`. RTOS/bare metal can supply a caller-provided clock callback.
- **1D/2D/3D:** vectors use a shared adaptive cutoff derived from vector speed, so the axes retain consistent response when units match.
- **Three tunable parameters:** minimum cutoff, speed coefficient `beta`, derivative cutoff.
- Documented error codes, transactional failure behavior, no heap/global mutable state, MIT license, cross-platform CI and tests.

## Quick start: 2D auto-timed coordinates

```c
#include "one_euro_filter.h"
#include <stdio.h>

int main(void)
{
    oef_auto_filter2d_t filter;
    oef_vec2_t input = {100.5f, 200.3f};
    oef_vec2_t output;

    /* First NULL selects the default config, second NULL the OS clock. */
    oef_status_t status = oef_auto_init2d(&filter, NULL, NULL, NULL);
    if (status != OEF_OK) return 1;

    /* Call for each newly sampled coordinate. */
    status = oef_auto_update2d(&filter, input, &output);
    if (status != OEF_OK) {
        fprintf(stderr, "%s: %s\n", oef_status_name(status),
                oef_status_message(status));
        return 1;
    }
    printf("(%.3f, %.3f)\n", output.x, output.y);
    return 0;
}
```

The 1D/2D examples use the built-in OS clock; the loop's short sleep stands in for a real sensor's sample spacing. The 3D example injects a custom clock callback, as you would on an RTOS.

**Runnable examples** (each shows the integration pattern — one filter, one update call per sample — with a realistic noisy input): [1D temperature-like sensor](examples/one_dimensional.c) · [2D jittery pointer coordinates](examples/two_dimensional.c) · [3D tracker with a custom RTOS clock](examples/three_dimensional.c).

### 1D auto timing

```c
oef_auto_filter_t filter;
oef_auto_init(&filter, NULL, NULL, NULL);
float filtered;
oef_status_t st = oef_auto_update(&filter, 25.12f, &filtered);
```

### 3D auto timing

```c
oef_auto_filter3d_t filter;
oef_auto_init3d(&filter, NULL, NULL, NULL);
oef_vec3_t input = {1.02f, 1.98f, 3.01f}, filtered;
oef_status_t st = oef_auto_update3d(&filter, input, &filtered);
```

### Explicit elapsed time (no system clock)

If your sensor provides timestamps, calculate the elapsed interval in **seconds** from consecutive captured samples, then call the core directly:

```c
oef_filter2d_t filter;
oef_init2d(&filter, NULL);
/* dt_seconds comes from the sensor timestamps, not this library. */
oef_vec2_t input = {100.5f, 200.3f}, filtered;
oef_status_t st = oef_update2d(&filter, input, 0.023f, &filtered);
```

This API is often **more accurate** than reading time at the processing thread if acquisition and processing can be delayed or queued.

## How One Euro Filter works

An ordinary first-order low-pass filter uses one smoothing coefficient, forcing a trade-off between stationary stability and motion delay:

\[
\hat{x}_t = \hat{x}_{t-1} + \alpha (x_t - \hat{x}_{t-1})
\]

The *One Euro Filter* adapts the smoothing coefficient based on the estimated **movement speed**. Each sample follows five steps:

1. **Estimate raw velocity** using the elapsed time (seconds):

   \[
   v_t = \frac{x_t-x_{t-1}}{\Delta t}
   \]

2. **Smooth velocity** with derivative cutoff `derivative_cutoff_hz` (= \(f_d\)):

   \[
   \alpha_d = \frac{2\pi f_d\Delta t}{1+2\pi f_d\Delta t},\quad
   \hat{v}_t=\hat{v}_{t-1}+\alpha_d(v_t-\hat{v}_{t-1})
   \]

3. **Compute adaptive cutoff** from the filtered speed:

   \[
   f_{c,t}=f_{\min}+\beta\lvert\hat{v}_t\rvert
   \]

4. **Convert cutoff to a dynamic coefficient**:

   \[
   \alpha_t=\frac{2\pi f_{c,t}\Delta t}{1+2\pi f_{c,t}\Delta t}
   \]

5. **Smooth the value**:

   \[
   \hat{x}_t=\hat{x}_{t-1}+\alpha_t(x_t-\hat{x}_{t-1})
   \]

For 2D/3D the velocity vector is smoothed per axis, then its Euclidean magnitude \(\sqrt{v_x^2+v_y^2}\) or \(\sqrt{v_x^2+v_y^2+v_z^2}\) determines **one shared** \(\alpha_t\) applied to all axes. Axes should represent the same physical unit; otherwise use independent 1D filters.

**Low speed → small cutoff → stronger smoothing. High speed → larger cutoff → faster response.** This does *not* force a stationary output to remain exactly fixed, and does not correct jumps in an upstream measurement algorithm.

## Configuration

`oef_config_default()` provides illustrative starting values; passing `NULL` for the config argument anywhere selects these defaults:

| Field | Default | Valid range | Description |
| --- | ---: | --- | --- |
| `min_cutoff_hz` | `0.8` | finite, > 0 | Lower value = more smoothing at rest, more lag. |
| `beta` | `0.5` | finite, ≥ 0 | Higher value = faster response to movement; unit-dependent. |
| `derivative_cutoff_hz` | `1.0` | finite, > 0 | Low-pass cutoff for velocity estimation. |

There is no sampling-frequency field and no maximum-cutoff field. Cutoff frequencies are *filter parameters*, **not input sampling frequency**. Time intervals are in seconds, velocities in input units per second, and `beta` depends on the units used (pixels vs meters, for example).

A practical tuning sequence: set `beta=0`, adjust minimum cutoff until stationary noise is acceptable; increase `beta` for moving response; adjust derivative cutoff if the speed estimate is too noisy or sluggish.

## Automatic time semantics and RTOS integration

Automatic filters store the **last successful sample timestamp** separately from the filter's numerical state. The **first valid sample passes through** and stores its time. On any unsuccessful update, neither history nor timestamp nor output is changed; a later sample will span the whole interval since the last successful one. Non-advancing, backwards or non-finite clock readings return `OEF_E_TIME`. Call `oef_auto_reset*()` after stream restart or large pauses to restart the time baseline. Do not call the underlying filter's reset alone when using the auto wrapper.

On embedded systems, implement a high-resolution **monotonic** callback. Its `double` return value is seconds:

```c
/* Replace tick_count() / ticks_per_second with your actual RTOS API. */
typedef struct { double ticks_per_second; } clock_config_t;

double my_rtos_clock(void *ctx)
{
    clock_config_t *c = (clock_config_t *)ctx;
    /* Ensure the underlying tick counter handles wraparound. */
    return (double)tick_count() / c->ticks_per_second;
}

clock_config_t timer = {1000.0};
oef_auto_filter2d_t tracker;
oef_auto_init2d(&tracker, NULL, my_rtos_clock, &timer);
```

`tick_count()` is **pseudocode**: provide a real tick counter and rollover/overflow handling for your target. On unsupported hosts the built-in clock returns `0.0`; supply a callback there. A monotonic clock is required; **do not use local time, UTC time, or a clock subject to NTP/user adjustments**.

For automatic mode the clock must represent **when the sample was acquired**, or as close to that time as possible; for asynchronous pipelines with acquisition timestamps, prefer the explicit `dt` API.

## API and errors

| Action | Scalar | 2D | 3D |
| --- | --- | --- | --- |
| Explicit elapsed time | `oef_update` | `oef_update2d` | `oef_update3d` |
| Auto-clock initialization | `oef_auto_init` | `oef_auto_init2d` | `oef_auto_init3d` |
| Auto-clock update | `oef_auto_update` | `oef_auto_update2d` | `oef_auto_update3d` |
| Auto-clock reset | `oef_auto_reset` | `oef_auto_reset2d` | `oef_auto_reset3d` |
| Core filter initialization | `oef_init` | `oef_init2d` | `oef_init3d` |
| Core configuration | `oef_set_config` | `oef_set_config2d` | `oef_set_config3d` |

Status codes: `OEF_OK = 0`; `OEF_E_NULL = -1`; `OEF_E_CONFIG = -2`; `OEF_E_INPUT = -3`; `OEF_E_TIME = -4`. Use `oef_status_name()` / `oef_status_message()` for diagnostic strings. See [docs/API.md](docs/API.md) for detailed contracts.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

The simplest integration needs no build system at all: copy `include/one_euro_filter.h` plus `src/one_euro_filter.c`, `src/one_euro_auto.c` and `src/one_euro_clock.c` into your project and compile them. If you only use explicit-`dt` APIs, `src/one_euro_filter.c` alone is sufficient.

For strict code checks, compile with `-std=c99 -Wall -Wextra -Wpedantic -Werror`; use `-DOEF_ENABLE_SANITIZERS=ON` with supported GCC/Clang compilers. See [CONTRIBUTING.md](CONTRIBUTING.md) and [CHANGELOG.md](CHANGELOG.md).

## Migration from v2

**This is a breaking change.** The main renames and removals:

```c
/* v2: oef_auto_init2d_ex(&filter, &config, clock, ctx); */
/* v3:  oef_auto_init2d(&filter, &config_or_NULL, clock, ctx); */

/* v2: oef_update2d_ex(&filter, point, dt, &output); */
/* v3:  oef_update2d(&filter, point, dt, &output); */

/* v2 bool wrappers oef_init/oef_update/... removed; use the status API. */
/* v2 config field max_cutoff_hz removed; no replacement needed. */
/* v2 error codes OEF_E_STATE, OEF_E_RANGE, OEF_E_CLOCK removed. */
```

Rebuild all consuming software against v3 headers and library.

## References and license

The original algorithm was introduced by Casiez, Roussel and Vogel in *1€ Filter: A Simple Speed-based Low-pass Filter for Noisy Input in Interactive Systems* (CHI 2012). [Authors' reference page](https://cristal.univ-lille.fr/~casiez/1euro/).

[MIT License](LICENSE).
