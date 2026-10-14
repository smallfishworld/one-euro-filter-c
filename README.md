# one-euro-filter-c

[![C CI](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml/badge.svg)](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

**English** | [简体中文](README.zh-CN.md)

A small, configurable **One Euro Filter (1€ Filter)** library written in **pure C99**. It reduces high-frequency noise in sampled values and coordinates while adapting to fast motion, for applications such as sensor readings, pointer tracking and 2D/3D position measurements.

- **1D / 2D / 3D** APIs; each filter instance owns its own state.
- **Configurable** minimum cutoff, speed coefficient, derivative cutoff, sampling frequency and optional maximum cutoff.
- Fixed-frequency **`oef_step*()`** and variable-frequency **`oef_update*()`** APIs.
- Reconfigure and reset at runtime; rejects non-finite values and invalid parameters without modifying the filter state or output.
- **No allocation, OS calls, or external libraries** (apart from C math functions). Supports embedded/RTOS/bare-metal systems, Linux, Windows and macOS.
- CMake, Makefile, unit tests, independent runnable examples for all three dimensions, and CI.

## 1. How it works

### 1.1 Why not use an ordinary low-pass filter?

An ordinary first-order low-pass filter is:

```math
y_t = (1-\alpha)y_{t-1} + \alpha x_t
```

Here `x_t` is the new raw sample, `y_t` is the filtered sample, and `0 < \alpha < 1` controls the trade-off:

- **Small `\alpha`:** less jitter when nearly stationary, but more delay when the input moves.
- **Large `\alpha`:** faster response to movement, but more noise gets through.

The **One Euro Filter** adjusts `\alpha` every sample based on the **estimated input speed**. Slow signals are strongly smoothed; moving signals get a higher cutoff and less lag.

### 1.2 Step-by-step equations

Let `x_t` be the current sample and `\Delta t` the elapsed time in **seconds**. Each instance retains the previous raw sample, filtered sample, and filtered derivative.

**Step 1 — estimate velocity:**

```math
v_t = \frac{x_t - x_{t-1}}{\Delta t}
```

**Step 2 — low-pass the velocity** (to prevent measurement noise from looking like sudden movement):

```math
\alpha_d = \frac{2\pi f_d\Delta t}{1 + 2\pi f_d\Delta t}
```

```math
\hat v_t = \hat v_{t-1} + \alpha_d(v_t-\hat v_{t-1})
```

Here `f_d` is `derivative_cutoff_hz`.

**Step 3 — compute the adaptive cutoff:**

```math
f_{c,t} = f_{\min} + \beta |\hat v_t|
```

- `f_{\min}` = `min_cutoff_hz`, the cutoff when nearly stationary.
- `\beta` = `beta`, how quickly the cutoff rises with velocity.
- If `max_cutoff_hz > 0`, clamp `f_{c,t}` to that maximum.

**Step 4 — turn cutoff into smoothing coefficient:**

```math
\alpha_t = \frac{2\pi f_{c,t}\Delta t}{1 + 2\pi f_{c,t}\Delta t}
```

This is equivalent to `\alpha = \Delta t/(\Delta t + 1/(2\pi f_c))`.

**Step 5 — filter the new sample:**

```math
\hat x_t = \hat x_{t-1} + \alpha_t(x_t-\hat x_{t-1})
```

The **first valid sample passes through unchanged**, and the derivative is initialized to zero.

### 1.3 What changes for 2D and 3D?

This library uses a **shared adaptive cutoff for all axes** of a vector rather than applying two or three unrelated scalar filters.

For 2D, estimate/filter `v_x` and `v_y` as above, then use:

```math
s_t = \sqrt{\hat v_x^2+\hat v_y^2}
```

For 3D:

```math
s_t = \sqrt{\hat v_x^2+\hat v_y^2+\hat v_z^2}
```

Substitute `s_t` for `|\hat v_t|` in the adaptive cutoff equation. The resulting **same `\alpha_t` is applied to every coordinate**. This makes the filter's response consistent across directions when all axes share the same units. If your axes have different units or need different tuning, use independent 1D filters instead.

### 1.4 Illustrative example

At 60 samples/s (`\Delta t \approx 0.0167` s):

| Cutoff `f_c` | Approximate `\alpha` | Meaning |
| --- | ---: | --- |
| 0.8 Hz | 0.077 | Strong smoothing, more lag |
| 8 Hz | 0.456 | Weaker smoothing, faster response |

These values demonstrate the trade-off; they are not fixed settings. The actual adaptive cutoff depends on velocity, `beta` and any configured cap.

## 2. Getting started

Include the public header:

```c
#include "one_euro_filter.h"
```

### 2.1 1D — noisy scalar measurements

```c
oef_config_t cfg = oef_config_default();
cfg.sample_frequency_hz = 50.0f;

oef_filter_t filter;
if (!oef_init(&filter, &cfg)) {
    /* handle invalid configuration */
}

float filtered;
if (oef_step(&filter, 25.12f, &filtered)) {
    /* use filtered value */
}
```

Runnable source: [examples/one_dimensional.c](examples/one_dimensional.c).

### 2.2 2D — tracking (x, y) coordinates

```c
oef_config_t cfg = oef_config_default();
cfg.sample_frequency_hz = 60.0f;
cfg.min_cutoff_hz = 0.8f;
cfg.beta = 0.05f; /* example for pixel-based coordinates */

oef_filter2d_t filter;
if (!oef_init2d(&filter, &cfg)) {
    /* handle invalid configuration */
}

oef_vec2_t raw = {100.7f, 199.6f};
oef_vec2_t smooth;
if (oef_step2d(&filter, raw, &smooth)) {
    /* use smooth.x and smooth.y */
}
```

Runnable source: [examples/two_dimensional.c](examples/two_dimensional.c).

### 2.3 3D — spatial (x, y, z) coordinates

```c
oef_config_t cfg = oef_config_default();
cfg.max_cutoff_hz = 8.0f; /* optional maximum cutoff */

oef_filter3d_t filter;
if (!oef_init3d(&filter, &cfg)) {
    /* handle invalid configuration */
}

oef_vec3_t raw = {1.02f, 1.98f, 3.01f};
oef_vec3_t smooth;
float dt_seconds = 0.021f; /* real time since preceding sample */
if (oef_update3d(&filter, raw, dt_seconds, &smooth)) {
    /* use smooth.x, smooth.y and smooth.z */
}
```

Runnable source: [examples/three_dimensional.c](examples/three_dimensional.c).

## 3. Configuration

`oef_config_default()` initializes all available fields with **starting values** (not universal best settings):

| Field | Default | Valid range | Function |
| --- | ---: | --- | --- |
| `min_cutoff_hz` | `0.8` | finite, > 0 | Small = smoother at rest, with more lag. |
| `beta` | `0.5` | finite, ≥ 0 | Large = quicker motion response, but may amplify velocity-related noise. |
| `derivative_cutoff_hz` | `1.0` | finite, > 0 | Smooths the velocity estimate. |
| `sample_frequency_hz` | `25.0` | finite, > 0 | Only used by fixed-rate `oef_step*()` functions. |
| `max_cutoff_hz` | `0.0` | 0, or finite ≥ `min_cutoff_hz` | Optional speed-response cap in Hz; 0 disables it. |

**Units matter.** Cutoff frequencies are in Hz, and `dt_seconds` is in seconds. Velocity uses **input units per second**, so the numerical tuning of `beta` depends on those units. A filter receiving pixel coordinates cannot necessarily reuse the same `beta` as a filter receiving meters.

**Tuning procedure:**

1. Set `beta = 0` (fixed low-pass response). Adjust `min_cutoff_hz` until stationary noise is acceptable.
2. Increase `beta` incrementally until moving inputs respond quickly enough.
3. Tune `derivative_cutoff_hz` if the adaptive cutoff reacts too abruptly or too sluggishly to short bursts of motion.
4. Optionally set `max_cutoff_hz` to limit the maximum speed response. Test with representative real data, not just one input sequence.

## 4. API and behavior

| Purpose | 1D | 2D | 3D |
| --- | --- | --- | --- |
| Initialize | `oef_init` | `oef_init2d` | `oef_init3d` |
| Update with elapsed time | `oef_update` | `oef_update2d` | `oef_update3d` |
| Update at configured rate | `oef_step` | `oef_step2d` | `oef_step3d` |
| Modify parameters | `oef_set_config` | `oef_set_config2d` | `oef_set_config3d` |
| Reset history | `oef_reset` | `oef_reset2d` | `oef_reset3d` |

- `init`, `set_config`, `update` and `step` return `bool`. On an invalid parameter, `dt` or non-finite input/intermediate value they return `false` without updating state or output.
- `set_config` preserves history. `reset` clears history while retaining the configuration; the next valid sample passes through unchanged.
- For **variable sampling intervals**, use `oef_update*()` with real positive `dt_seconds`; `sample_frequency_hz` does not affect these calls. Avoid integer-division mistakes when computing seconds.
- No internal locking: **each instance can be used independently**, but protect an instance shared across threads.
- Use `reset` after changing coordinate units, switching to an unrelated signal, or intentionally discarding prior tracking history.
- The filter reduces *temporal* jitter; it does not fix outliers, inconsistent measurement models, or discontinuities in the raw data. It does not mathematically force stationary output to be constant.

## 5. Build and run

### CMake (Linux / macOS / Windows)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

CTest runs the library unit tests **and** smoke-tests all three independent examples.

Executable names: `oef_example_1d`, `oef_example_2d`, `oef_example_3d`.

On Linux/macOS you can run, for example:

```sh
./build/oef_example_1d
./build/oef_example_2d
./build/oef_example_3d
```

With a multi-config generator on Windows, executables commonly live in `build/Debug/`.

### Makefile (Unix-like systems)

```sh
make
make test
make example   # builds and runs all three examples
```

### Embed without a build system

Compile `src/one_euro_filter.c` and add `include/` to the compiler's include search path. Some platforms need `-lm` for the math library.

### Install / link with CMake

```sh
cmake --install build --prefix /your/install/prefix
```

```cmake
find_package(one_euro_filter 1.0 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE one_euro_filter::one_euro_filter)
```

## 6. References and license

The algorithm was introduced by Casiez, Roussel and Vogel in *1€ Filter: A Simple Speed-based Low-pass Filter for Noisy Input in Interactive Systems* (CHI 2012). See the [authors' 1€ Filter page](https://cristal.univ-lille.fr/~casiez/1euro/) for the paper and original materials.

The scalar formulation follows the standard One Euro Filter idea. The **shared vector-speed cutoff for 2D/3D** and optional **maximum cutoff** are implementation choices of this library.

Released under the [MIT License](LICENSE).
