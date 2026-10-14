# Changelog

This project follows [Semantic Versioning](https://semver.org/).

## [3.0.0] - 2026-10-14

### Breaking changes
- Removed the `_ex` suffixes: the status-returning functions are now the plain
  names (`oef_update`, `oef_auto_init2d`, ...).
- Removed all `bool` convenience wrappers.
- Removed `max_cutoff_hz` from `oef_config_t`; the standard three parameters remain.
- Removed the `OEF_E_STATE`, `OEF_E_RANGE` and `OEF_E_CLOCK` status codes;
  unusable clock readings now surface as `OEF_E_TIME`.
- The clock callback is now `double (*)(void *context)`; `oef_clock_monotonic()`
  returns `double` directly and returns `0.0` on unsupported platforms.
- Removed per-sample defensive state re-validation; documented that state
  written by the library is trusted after `oef_init*()`.
- Removed the standalone Makefile; CMake is the single build system.

### Added
- `NULL` config arguments now select `oef_config_default()` everywhere, making
  a minimal integration three lines.
- README documents copy-the-files integration without any build system.

## [2.0.0] - 2026-10-14

### Breaking changes
- Removed `sample_frequency_hz` from `oef_config_t`; the sampling rate no longer needs to be configured.
- Removed fixed-frequency `oef_step*()` and `oef_step*_ex()` calls. Use automatic monotonic-clock wrappers or manual `dt` updates.
- Rebuild applications against v2 headers and library; configuration layout and static symbols changed.

### Added
- `oef_auto_filter_t`, `oef_auto_filter2d_t`, `oef_auto_filter3d_t`: automatic timing with caller-owned clock state.
- `oef_auto_init*_ex`, `oef_auto_update*_ex` and `oef_auto_reset*_ex` APIs.
- Built-in OS monotonic clock adapter (Linux/macOS `clock_gettime`, Windows `QueryPerformanceCounter`) and RTOS callback injection.
- `OEF_E_CLOCK` error code and dedicated deterministic automatic-clock tests.
- Updated 1D/2D/3D examples, full English/Chinese READMEs and API contract documentation.

### Retained
- 1D/2D/3D numerical filtering, core manual-`dt` API, error-code interface and bool compatibility wrappers for manual updates.

## [1.1.0] - 2026-10-14

- Added Doxygen API documentation and stable status-returning `_ex` interfaces.
- Hardened numerical core with double-precision intermediates and transactional failure handling.
- Added edge case/stress tests and sanitizer CI.

## [1.0.0] - 2026-10-14

- Initial C99 scalar and vector One Euro Filter implementation.
