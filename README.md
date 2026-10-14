# one-euro-filter-c

[![C CI](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml/badge.svg)](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

**English** | [简体中文](docs/README.zh-CN.md)

Tiny, configurable [One Euro Filter (1€ Filter)](https://cristal.univ-lille.fr/~casiez/1euro/) implementation in pure **C99**. Smooth jitter while preserving a fast response when the signal moves. Good for acoustic-camera peak coordinates, touch/pointer tracking, and sensor signals.

## Features

- **1D scalar**, **2D point**, and **3D point** filters.
- 2D/3D filters calculate **one shared adaptive cutoff** from vector velocity to keep axis response consistent. The scalar API follows the standard 1€ formulation.
- Fully configurable `min_cutoff_hz`, `beta`, `derivative_cutoff_hz`, `sample_frequency_hz`, and optional `max_cutoff_hz`.
- Fixed-rate `oef_step*()` or real elapsed time `oef_update*()` in seconds.
- Runtime parameter updates, explicit reset, input/parameter validation.
- Pure C99; **no heap allocations, threads, operating-system calls or runtime dependencies** beyond the standard C math library. Works with Linux, Windows, macOS, RTOS and bare-metal devices.
- CMake/Makefile, installable CMake target, unit tests and acoustic imaging demo.

## Quick start (2D acoustic source position)

```c
#include "one_euro_filter.h"

static oef_filter2d_t source_filter;

void source_init(void)
{
    oef_config_t cfg = oef_config_default();
    cfg.min_cutoff_hz = 0.8f;      /* reduce static jitter */
    cfg.beta = 0.5f;               /* respond faster during movement */
    cfg.derivative_cutoff_hz = 1.0f;
    cfg.sample_frequency_hz = 25.0f;
    cfg.max_cutoff_hz = 0.0f;      /* 0 means unlimited */
    (void)oef_init2d(&source_filter, &cfg);
}

void on_sound_frame(float subgrid_x, float subgrid_y, float dt_seconds)
{
    oef_vec2_t raw = {subgrid_x, subgrid_y};
    oef_vec2_t stable;
    if (oef_update2d(&source_filter, raw, dt_seconds, &stable)) {
        /* Use stable.x/y to generate the cloud AND draw the TOP marker. */
        /* Example: screen_x = stable.x * 640.0f / 40.0f; */
        /*          screen_y = stable.y * 480.0f / 32.0f; */
    }
}
```

For a strictly fixed 25 Hz frame rate use `oef_step2d(&source_filter, raw, &stable)`. To switch tracked sources or coordinate units, use `oef_reset2d(&source_filter)`.

### 1D and 3D

```c
/* Once at initialization */
oef_config_t cfg = oef_config_default();
oef_filter_t scalar;
oef_filter3d_t point3;
oef_init(&scalar, &cfg);
oef_init3d(&point3, &cfg);

/* On each update; pass actual elapsed time in seconds */
float result;
oef_vec3_t point = {1.0f, 2.0f, 3.0f}, filtered;
oef_update(&scalar, 42.0f, 0.04f, &result);
oef_update3d(&point3, point, 0.04f, &filtered);
```

Each filter instance stores its own state and configuration. Instances can be used on separate threads; sharing **one** instance across threads needs external synchronization.

## Configuration

| Field | Allowed values | Description |
| --- | --- | --- |
| `min_cutoff_hz` | finite, > 0 | Minimum signal cutoff. Lower values stabilize a stationary signal but increase lag. |
| `beta` | finite, >= 0 | Speed response. Larger values reduce lag during movement but can let jitter through. Value depends on input coordinate units. |
| `derivative_cutoff_hz` | finite, > 0 | Low-pass cutoff for the estimated speed; common starting point: 1 Hz. |
| `sample_frequency_hz` | finite, > 0 | Nominal Hz for `oef_step*()` only; `oef_update*()` uses caller-provided `dt_seconds`. |
| `max_cutoff_hz` | 0 or >= `min_cutoff_hz` | Optional adaptive cutoff cap; zero disables it. |

`oef_config_default()` returns `{0.8, 0.5, 1.0, 25.0, 0.0}`. These are **example starting values, not universal calibration**. All frequencies are in hertz. Internally, `cutoff = min_cutoff + beta * |filtered_velocity|`. The effective dimension of `beta` is inverse input units. The algorithm uses an adaptive first-order low-pass with `alpha = 2π f dt / (1 + 2π f dt)`.

## API

| Operation | 1D | 2D | 3D |
| --- | --- | --- | --- |
| Initialize | `oef_init` | `oef_init2d` | `oef_init3d` |
| Sample with real `dt` | `oef_update` | `oef_update2d` | `oef_update3d` |
| Sample with configured Hz | `oef_step` | `oef_step2d` | `oef_step3d` |
| Reconfigure (keep history) | `oef_set_config` | `oef_set_config2d` | `oef_set_config3d` |
| Reset history | `oef_reset` | `oef_reset2d` | `oef_reset3d` |

`init`, `update`, `step` and `set_config` return `bool`. Invalid configuration, non-finite input, invalid `dt` or non-finite intermediate values yield `false`, preserving the state and output. First valid input is returned unchanged. `reset` clears the history but retains configuration. Keep `dt` positive and accurate. After a long pause, decide whether to reset to avoid an unintended jump.

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/oef_acoustic_example
```

On Unix-like systems you may also run `make`, `make test`, and `make example`.

To integrate into an embedded project, simply compile `src/one_euro_filter.c` and add `include/` to include paths. On some platforms link `-lm`.

To install the CMake package: `cmake --install build --prefix /some/prefix`. Then use:

```cmake
find_package(one_euro_filter 1.0 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE one_euro_filter::one_euro_filter)
```

## Important for acoustic heatmaps

Filter sub-grid floating-point peak coordinates **before generating** the heatmap, not the enlarged 1280×960 image. Generate the cloud and TOP marker using the *same* filtered coordinates. 1€ filtering cannot repair discontinuities in peak interpolation when TOP switches cells, or remove fluctuations in the acoustic MAP's intensity/shape. It also does not lock a stationary position exactly; an optional small display deadband may be used if necessary. For diagnostic work, compare raw, filtered and displayed positions separately.

## License

MIT — see [LICENSE](LICENSE).
