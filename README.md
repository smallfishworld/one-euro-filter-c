# one-euro-filter-c

[![C CI](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml/badge.svg)](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

**English** | [简体中文](README.zh-CN.md)

A **small, allocation-free C99 library** implementing the *One Euro Filter* for 1D values, 2D points, and 3D positions. Smooth stationary measurement jitter while responding more quickly to meaningful movement. Suitable for sensors, UI pointers and tracking data.

Version **2.0.0** removes the fixed-frequency configuration. You can now choose **automatic monotonic-clock timing** or **explicit per-sample elapsed time**. No fixed Hz value needs to be configured.

## Features

- **Automatic timing:** `oef_auto_update*_ex()` reads a monotonic clock and derives elapsed seconds between *successfully processed* samples.
- **Explicit timing:** `oef_update*_ex(..., dt_seconds, ...)` for externally timestamped measurements or deterministic processing.
- **Platform clock:** Linux/macOS use `clock_gettime(CLOCK_MONOTONIC)`; Windows uses `QueryPerformanceCounter`. RTOS/bare metal can supply a caller-provided clock callback.
- **1D/2D/3D:** vectors use a shared adaptive cutoff derived from vector speed, so the axes retain consistent response when units match.
- **Configurable parameters:** minimum cutoff, speed coefficient `beta`, derivative cutoff, optional maximum cutoff.
- Doxygen API comments, documented error codes, transactional failure behavior, no heap/global mutable state, MIT license, cross-platform CI and tests.

## Quick start: 2D auto-timed coordinates

```c
#include "one_euro_filter.h"
#include <stdio.h>

int main(void)
{
    oef_config_t config = oef_config_default();
    oef_auto_filter2d_t filter;
    oef_vec2_t input = {100.5f, 200.3f};
    oef_vec2_t output;

    /* NULL uses the built-in OS monotonic clock. No frequency or dt. */
    oef_status_t status = oef_auto_init2d_ex(&filter, &config, NULL, NULL);
    if (status != OEF_OK) return 1;

    /* Call for each newly sampled coordinate. */
    status = oef_auto_update2d_ex(&filter, input, &output);
    if (status != OEF_OK) {
        fprintf(stderr, "%s: %s\n", oef_status_name(status),
                oef_status_message(status));
        return 1;
    }
    printf("(%.3f, %.3f)\n", output.x, output.y);
    return 0;
}
```

The runnable examples use a **deterministic simulated monotonic clock** so that very fast consecutive demonstration calls still have positive elapsed time; production code can use the default OS clock by passing `NULL`.

**Runnable, generic examples:** [1D scalar](examples/one_dimensional.c) · [2D XY](examples/two_dimensional.c) · [3D XYZ with injected clock](examples/three_dimensional.c).

### 1D auto timing

```c
oef_config_t config = oef_config_default();
oef_auto_filter_t filter;
oef_auto_init_ex(&filter, &config, NULL, NULL);
float filtered;
oef_status_t st = oef_auto_update_ex(&filter, 25.12f, &filtered);
```

### 3D auto timing

```c
oef_config_t config = oef_config_default();
oef_auto_filter3d_t filter;
oef_auto_init3d_ex(&filter, &config, NULL, NULL);
oef_vec3_t input = {1.02f, 1.98f, 3.01f}, filtered;
oef_status_t st = oef_auto_update3d_ex(&filter, input, &filtered);
```

### Explicit elapsed time (no system clock)

If your sensor provides timestamps, calculate the elapsed interval in **seconds** from consecutive captured samples, then call the core directly:

```c
oef_config_t config = oef_config_default();
oef_filter2d_t filter;
oef_init2d_ex(&filter, &config);
/* dt_seconds comes from the sensor timestamps, not this library. */
oef_vec2_t input = {100.5f, 200.3f}, filtered;
oef_status_t st = oef_update2d_ex(&filter, input, 0.023f, &filtered);
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

   If `max_cutoff_hz > 0`, clamp this value to that maximum.

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

`oef_config_default()` provides illustrative starting values:

| Field | Default | Valid range | Description |
| --- | ---: | --- | --- |
| `min_cutoff_hz` | `0.8` | finite, > 0 | Lower value = more smoothing at rest, more lag. |
| `beta` | `0.5` | finite, ≥ 0 | Higher value = faster response to movement; unit-dependent. |
| `derivative_cutoff_hz` | `1.0` | finite, > 0 | Low-pass cutoff for velocity estimation. |
| `max_cutoff_hz` | `0` | zero or finite ≥ min cutoff | Optional cap; zero disables cap. |

**There is no `sample_frequency_hz` field in v2.0.** Frequencies in the config are *cutoff frequencies* (filter parameters), **not input sampling frequency**. Time intervals are in seconds, velocities in input units per second, and `beta` depends on the units used (pixels vs meters, for example).

A practical tuning sequence: set `beta=0`, adjust minimum cutoff until stationary noise is acceptable; increase `beta` for moving response; adjust derivative cutoff if the speed estimate is too noisy or sluggish; cap the maximum cutoff if desired.

## Automatic time semantics and RTOS integration

Automatic filters store the **last successful sample timestamp** separately from the filter's numerical state. The **first valid sample passes through** and stores its time. On any unsuccessful update, neither history nor timestamp nor output is changed; a later sample will span the whole interval since the last successful one. Zero/backward times return `OEF_E_TIME`; invalid/failed clock reads return `OEF_E_CLOCK`; intervals not representable as positive `float` return `OEF_E_RANGE`. Call `oef_auto_reset*_ex()` after stream restart or large pauses to restart the time baseline. Do not call the underlying filter's reset alone when using the auto wrapper.

On embedded systems, implement a high-resolution **monotonic** callback. Its `double` result is in seconds, and the callback must fill `*seconds` and return `OEF_OK`:

```c
/* Replace tick_count() / ticks_per_second with your actual RTOS API. */
typedef struct { double ticks_per_second; } clock_config_t;

oef_status_t my_rtos_clock(void *ctx, double *seconds)
{
    clock_config_t *c = (clock_config_t *)ctx;
    if (c == NULL || seconds == NULL || c->ticks_per_second <= 0.0)
        return OEF_E_CLOCK;
    /* Ensure the underlying tick counter handles wraparound. */
    *seconds = (double)tick_count() / c->ticks_per_second;
    return OEF_OK;
}

clock_config_t timer = {1000.0};
oef_auto_filter2d_t tracker;
oef_config_t config = oef_config_default();
oef_auto_init2d_ex(&tracker, &config, my_rtos_clock, &timer);
```

`tick_count()` is **pseudocode**: provide a real tick counter and rollover/overflow handling for your target. On unsupported hosts, passing `NULL` as the clock callback returns `OEF_E_CLOCK` on update rather than falling back to wall time. A monotonic clock is required; **do not use local time, UTC time, or a clock subject to NTP/user adjustments**.

If the callback returns success but an invalid or unwritten timestamp, the library reports `OEF_E_CLOCK`. If the interval is 0 or negative, it reports `OEF_E_TIME`. For automatic mode the clock must represent **when the sample was acquired**, or as close to that time as possible; for asynchronous pipelines with acquisition timestamps, prefer the explicit `dt` API.

## API and errors

| Action | Scalar | 2D | 3D |
| --- | --- | --- | --- |
| Explicit elapsed time | `oef_update_ex` | `oef_update2d_ex` | `oef_update3d_ex` |
| Auto-clock initialization | `oef_auto_init_ex` | `oef_auto_init2d_ex` | `oef_auto_init3d_ex` |
| Auto-clock update | `oef_auto_update_ex` | `oef_auto_update2d_ex` | `oef_auto_update3d_ex` |
| Auto-clock reset | `oef_auto_reset_ex` | `oef_auto_reset2d_ex` | `oef_auto_reset3d_ex` |
| Core filter initialization | `oef_init_ex` | `oef_init2d_ex` | `oef_init3d_ex` |
| Core configuration | `oef_set_config_ex` | `oef_set_config2d_ex` | `oef_set_config3d_ex` |

Status codes: `OEF_OK = 0`; `OEF_E_NULL = -1`; `OEF_E_CONFIG = -2`; `OEF_E_INPUT = -3`; `OEF_E_TIME = -4`; `OEF_E_STATE = -5`; `OEF_E_RANGE = -6`; `OEF_E_CLOCK = -7`. Use `oef_status_name()` / `oef_status_message()` for diagnostic strings. Old `bool` manual-time wrappers remain available; fixed-frequency `oef_step*()` was removed with the frequency field. See [docs/API.md](docs/API.md) for detailed contracts.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

On Unix-like systems, `make test` and `make example` are also supported. Executables: `oef_example_1d`, `oef_example_2d`, and `oef_example_3d`. For embedded integration compile `src/one_euro_filter.c`, `src/one_euro_auto.c` and `src/one_euro_clock.c` with `include/`; `one_euro_clock.c` has an unsupported-platform stub when no native monotonic clock is available. You can also link only `src/one_euro_filter.c` if you exclusively use explicit-`dt` APIs.

For strict code checks, compile with `-std=c99 -Wall -Wextra -Wpedantic -Werror`; use `-DOEF_ENABLE_SANITIZERS=ON` with supported GCC/Clang compilers. See [CONTRIBUTING.md](CONTRIBUTING.md) and [CHANGELOG.md](CHANGELOG.md).

## Migration from v1.1

**This is a breaking change.** Replace old fixed-frequency calls:

```c
/* v1.1: cfg.sample_frequency_hz = 60.0f; */
/* v1.1: oef_step2d_ex(&filter, point, &output); */

/* v2: choose auto timing (no sample rate required) */
oef_auto_filter2d_t auto_filter;
oef_auto_init2d_ex(&auto_filter, &cfg, NULL, NULL);
oef_auto_update2d_ex(&auto_filter, point, &output);

/* OR choose explicit sampling interval (seconds): */
oef_filter2d_t timed_filter;
oef_init2d_ex(&timed_filter, &cfg);
oef_update2d_ex(&timed_filter, point, 1.0f/60.0f, &output);
```

Config layout changed from **five** fields to **four** and the fixed-frequency `oef_step*` APIs have been removed. Rebuild all consuming software against v2.0 headers and library. The original manual-time `oef_update*` APIs remain supported.

## References and license

The original algorithm was introduced by Casiez, Roussel and Vogel in *1€ Filter: A Simple Speed-based Low-pass Filter for Noisy Input in Interactive Systems* (CHI 2012). [Authors' reference page](https://cristal.univ-lille.fr/~casiez/1euro/).

[MIT License](LICENSE).
