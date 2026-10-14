# Changelog

This project follows [Semantic Versioning](https://semver.org/).

## [1.1.0] - 2026-10-14

### Added
- Doxygen documentation of exported structs, configuration fields, constants and functions.
- Stable library error codes (`oef_status_t`), symbolic names and readable error descriptions.
- Status-returning `*_ex` alternatives for initialization, configuration, reset, fixed-rate and variable-rate updates.
- Dedicated error-path and boundary-value tests, deterministic randomized stress tests, and sanitizer CI.
- API contracts, error handling and integration notes in both languages.

### Changed
- Refactored scalar/vector algorithms to share one numerical core using double-precision intermediates.
- Failed operations now preserve filter history and output; invalid retained state and unrepresentable numeric results are rejected.
- Three self-contained examples now demonstrate status codes and diagnostics.
- Version raised to 1.1.0 (source-compatible extension of 1.0 interfaces).

## [1.0.0] - 2026-10-14

- Initial C99 1D/2D/3D filters with runtime parameters, examples and CI.
