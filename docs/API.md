# One Euro Filter C — v3 API Contract

This page defines status codes, ownership, clock guarantees and edge cases. The library uses C99 and stores filter state in caller-provided structs.

## Filter parameters

`oef_config_t` contains exactly three tunable fields:

| Field | Default | Constraint | Notes |
| --- | ---: | --- | --- |
| `min_cutoff_hz` | 0.8 | positive, finite | Minimum signal cutoff in Hz |
| `beta` | 0.5 | nonnegative, finite | Cutoff increase per unit speed |
| `derivative_cutoff_hz` | 1.0 | positive, finite | Low-pass cutoff for velocity estimate |

**No sampling frequency is configured.** Use elapsed seconds or an injected/default monotonic clock. Axes in 2D/3D must have matching units for a meaningful velocity norm.

Every function that takes a `const oef_config_t *config` also accepts **NULL**, which selects `oef_config_default()`.

## Initialization and object lifetime

- Create a writable instance in caller-owned storage; call the matching `oef_init*()` or `oef_auto_init*()` before updating.
- The core `oef_filter_t`, `oef_filter2d_t`, `oef_filter3d_t` do not own a clock. They require explicit `float dt_seconds > 0` every update.
- Auto wrappers (`oef_auto_filter_t`, `oef_auto_filter2d_t`, `oef_auto_filter3d_t`) contain a core filter, clock callback, optional context and previous successful timestamp (`double`).
- Passing a `NULL` clock callback selects the built-in monotonic clock on Linux/macOS/Windows. Unsupported RTOS/bare-metal backends need an application-supplied callback.
- Call `oef_auto_reset*()` to reset BOTH core state and the saved timestamp. Alter tuning parameters on the embedded filter with `oef_set_config*(&auto.filter, &config)`; this keeps history and clock baseline.
- No malloc, global mutable last-error state or hidden worker thread. Separate instances are independent. Protect shared instances with external synchronization.
- Passing a pointer alias into internal instance fields as `output` is not supported. All parameters and callback contexts must remain valid throughout calls.

## Time model

- The clock callback signature is `double (*oef_clock_fn)(void *context)`. It must return a **finite, monotonically increasing timestamp in seconds**.
- The built-in clock uses `QueryPerformanceCounter` (Windows) or `clock_gettime(CLOCK_MONOTONIC)` (Linux/macOS). It never uses wall-clock/UTC time. On unsupported platforms it returns `0.0`.
- Initial successful sample is passed straight through and its timestamp becomes the baseline; no meaningful interval exists before it.
- On subsequent calls, `dt = (float)(now - last_successful_timestamp)`. A timestamp that does not advance, moves backward, or is non-finite returns `OEF_E_TIME`.
- If filtering fails, neither timestamp, filter history nor output advances. The clock callback itself may have side effects that cannot be undone.
- Acquisition timestamps are preferred over processing-call times for queued/delayed samples. Use explicit `dt` when you already have them.

## Status codes

| Status | Number | Meaning |
| --- | ---: | --- |
| `OEF_OK` | 0 | Success |
| `OEF_E_NULL` | -1 | Required pointer is null |
| `OEF_E_CONFIG` | -2 | Proposed configuration invalid |
| `OEF_E_INPUT` | -3 | Input sample contains NaN or infinity |
| `OEF_E_TIME` | -4 | Sampling interval <= 0 or nonfinite, or automatic time did not advance |

`oef_status_name` and `oef_status_message` return static strings. Error codes are NOT POSIX `errno`. On any failed init/reconfigure/update, the affected data structure (and output, if any) is unchanged; callback side effects lie outside this guarantee.

## Source/binary compatibility

v3.0 removes the v2 `_ex` suffixes (the status API is now the plain name), the `bool` convenience wrappers, the `max_cutoff_hz` config field, and the `OEF_E_STATE`, `OEF_E_RANGE`, `OEF_E_CLOCK` status codes. The clock callback now returns `double` directly. This is a **source and ABI breaking change** from v2.0. Affected callers must migrate and rebuild.

## Examples and checks

Examples: [1D](../examples/one_dimensional.c), [2D](../examples/two_dimensional.c), [3D custom-clock](../examples/three_dimensional.c).

Run `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`. The tests check core filtering, exact errors, deterministic callback timestamps, no mutation on failure, monotonic-clock backend and vectors. Optional `OEF_ENABLE_SANITIZERS=ON` adds ASan/UBSan with supported compilers.
