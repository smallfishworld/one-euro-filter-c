# Contributing

Thank you for improving the library. This is a small C99 component intended for Linux, Windows, macOS, RTOS and bare-metal environments.

- Keep the public header independent from OS-specific APIs; native clock integration belongs in `src/one_euro_clock.c`. Do not add heap allocation or runtime globals.
- Document every new public function, type, macro and field using Doxygen (`/** ... */`, `@param`, `@return`/`@retval`). Add comments for private helper functions, too.
- Introduce stable new status constants rather than reusing existing codes for different errors. Maintain existing `bool` compatibility wrappers.
- Fail without modifying history or output when encountering invalid input, invalid state, clock failures, non-increasing timestamps or numeric overflow. Never dereference a nullable pointer before checking it.
- Validate all user-configurable fields and document their units, ranges and defaults.
- Add unit tests for success paths, null pointers, bad timing, NaN/Inf and extreme values; include a regression test when fixing a bug.
- Format C with `.clang-format` when available. Keep `-std=c99 -Wall -Wextra -Wpedantic -Werror` builds clean on GCC and Clang.

## Local checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
make test
```

For supported GCC/Clang toolchains, optional sanitizers are available:

```sh
cmake -S . -B build-sanitize -DOEF_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

Please include API/behavior changes in `CHANGELOG.md` and update **both** `README.md` and `README.zh-CN.md`.
