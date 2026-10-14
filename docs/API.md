# One Euro Filter API Contracts (v1.1.0)

**Language:** C99. **Header:** `include/one_euro_filter.h`. **Ownership:** caller-owned instances. **Error policy:** `OEF_OK == 0`; negative, stable, library-defined codes (not POSIX `errno`).

## 1. Lifecycle

1. Create a writable `oef_filter_t`, `oef_filter2d_t` or `oef_filter3d_t` object with an adequate lifetime. It may be uninitialized at this point.
2. Obtain and customize `oef_config_default()`; all five fields are always validated.
3. Call `oef_init_ex()` / `oef_init2d_ex()` / `oef_init3d_ex()` **before calling any other instance API**. Successful initialization clears history.
4. For each finite input, use `oef_update*_ex()` with actual `dt_seconds > 0`, or `oef_step*_ex()` for uniform `sample_frequency_hz` sampling.
5. For tuning changes, call `oef_set_config*_ex()`; past data remains in the instance. To discard history after a signal source/unit change, use `oef_reset*_ex()`.
6. Release the object normally. No `destroy` function or heap allocation is necessary.

The first successful `update` after initialization/reset outputs its input unchanged. Invalid calls neither write an output nor advance any history. When an update fails, choose whether to retry, skip or reset; do not silently interpret a stale output as a fresh sample.

## 2. Status codes

| Code | Value | Meaning | Typical recovery |
|---|---:|---|---|
| `OEF_OK` | `0` | Success | Use the output |
| `OEF_E_NULL` | `-1` | A required pointer is `NULL` | Fix call site |
| `OEF_E_CONFIG` | `-2` | Candidate configuration invalid | Fix a field; no mutation |
| `OEF_E_INPUT` | `-3` | Raw input contains NaN/Inf | Discard invalid sensor sample |
| `OEF_E_TIME` | `-4` | `dt_seconds` is not finite and positive | Supply measured seconds or fixed-rate `step` |
| `OEF_E_STATE` | `-5` | Filter config/history invalid, or not yet initialized | Call `init` or investigate state corruption |
| `OEF_E_RANGE` | `-6` | Numeric result cannot fit a stored `float` | Inspect sample units and time scales; consider reset |

- `oef_status_name(code)` returns a static symbolic name.
- `oef_status_message(code)` returns a static human-readable diagnostic string.
- Unknown codes are reported as `OEF_UNKNOWN` / `unknown status code`.
- All error strings are static and safe to use without an allocator; no global last-error variable exists.
- **Error precedence:** null pointer checks occur before processing; otherwise input and time checks may precede current-state verification. Do not rely on a particular precedence when multiple arguments are invalid.

```c
#include "one_euro_filter.h"
#include <stdio.h>

void process_sample(oef_filter2d_t *tracker, oef_vec2_t input, float dt)
{
    oef_vec2_t output;
    oef_status_t st = oef_update2d_ex(tracker, input, dt, &output);
    if (st == OEF_OK) {
        /* Consume output.x / output.y. */
    } else {
        fprintf(stderr, "filter: %s (%s)\n",
                oef_status_name(st), oef_status_message(st));
    }
}
```

## 3. Configuration constraints

| Field | Required range | Default | Remarks |
|---|---|---:|---|
| `min_cutoff_hz` | finite, > 0 | 0.8 | Stationary smoothness |
| `beta` | finite, >= 0 | 0.5 | Motion response, depends on input units |
| `derivative_cutoff_hz` | finite, > 0 | 1.0 | Low-pass for estimated velocity |
| `sample_frequency_hz` | finite, > 0 | 25.0 | Only `step` APIs use this |
| `max_cutoff_hz` | 0 or finite >= min cutoff | 0 | Optional upper cap, zero = disabled |

Inputs must be finite; `update` also needs `dt_seconds > 0`, finite. `step` computes `dt_seconds = 1.0 / sample_frequency_hz`. If this reciprocal cannot be represented by a positive finite `float`, `OEF_E_RANGE` is returned before changing state.

The implementation computes critical differences, velocities, cutoffs and alpha in `double` precision before validating and converting back to `float`. This reduces spurious overflows from intermediate `float` arithmetic, at the cost of some cycles on microcontrollers without hardware double-precision support. It is **not** a promise to process arbitrarily large/discontinuous data: velocity retained as `float` must be representable.

## 4. Memory, concurrency and compatibility

- **Caller-owned memory:** library does not allocate, free, lock or use mutable global state.
- **State ownership:** `config` is readable; prefer `oef_set_config*_ex()` rather than direct struct mutation. Treat the `last_*` fields and `initialized` as read-only. Objects must not be concurrently changed, and output must not overlap instance storage.
- **Threading:** different instances can run on different threads. Sharing one instance between threads requires a mutex or exclusive ownership.
- **Lifecycle:** do not call `step`/`update`/`reset`/`set_config` on an uninitialized C object. A zero-filled object returns state errors rather than producing output, but an arbitrary uninitialized stack object is not a supported input.
- **State failure:** if an active configuration is corrupted, reset may fail with `OEF_E_STATE`; re-initialize from a known-good configuration to recover.
- **ABI/source compatibility:** v1.1 adds `*_ex` functions and an enum without removing or changing the signatures/field layout of the original v1.0 `bool`/`void` API. Those wrappers still work: `bool` functions return `true` exactly for `OEF_OK`; `void` reset wrappers discard status.
- **Coordinate models:** scalar applies the traditional speed-adaptive coefficient; 2D/3D use one shared cutoff from the norm of filtered velocity. For heterogeneous coordinate units, use separate 1D filters.

## 5. Quality assurance

`tests/test_one_euro_filter.c` retains the v1.0 behavior tests. `tests/test_error_codes.c` adds error enumeration, transaction/rollback behavior, invalid configuration, NaN/Inf, extreme sampling periods, numerical range, state corruption, and deterministic 25,000-sample stress checks. All three examples are smoke-tested by CTest. The CI matrix runs on Windows, macOS and Ubuntu, plus an AddressSanitizer/UndefinedBehaviorSanitizer job on Linux.

Further details and mathematical derivations are in [English README](../README.md) and [中文 README](../README.zh-CN.md). Style guidance: [Zephyr API design](https://docs.zephyrproject.org/latest/develop/api/design_guidelines.html), [Zephyr Doxygen](https://docs.zephyrproject.org/latest/contribute/style/doxygen.html), [libuv errors](https://docs.libuv.org/en/stable/errors.html).
