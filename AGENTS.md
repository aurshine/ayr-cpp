# Repository Guidelines

## Project Structure & Modules

`ayr-cpp` is a C++20 header-only library designed to be lightweight, clear in design, efficient at runtime, and simple to use. It aims to minimize external dependencies and work out of the box. Its APIs should remain easy to extend while providing broad functionality. Public headers live in `include/ayr/`, grouped by module: `base`, `async`, `coro`, `fs`, `json`, and `net`. The umbrella headers (such as `include/ayr/base.hpp`) expose module APIs. Tests are standalone programs in `test/`; JSON fixtures are under `test/json/`. CMake build output belongs in `out/` and should not be committed. `StyleGuide.md` documents the detailed C++ conventions.

## Build, Test & Development

Requires CMake 3.28+, a C++20 compiler, and OpenSSL. fmt is optional when the standard library provides `<format>`.

```bash
cmake -S . -B out/build -DBUILD_TESTING=ON
cmake --build out/build --parallel
ctest --test-dir out/build --output-on-failure
```

For Visual Studio or another multi-configuration generator, add `--config Debug` to build and `-C Debug` to CTest. The repository wrappers rebuild and run the suite: `bash ./RunTest.sh Debug` or `./RunTest.ps1 -Config Debug`. GCC/Clang coverage instrumentation is available with `-DAYR_ENABLE_COVERAGE=ON`.

## Coding Style & Naming

Use four spaces and follow `StyleGuide.md`: snake_case for functions and variables, trailing underscores for members, PascalCase for types, and uppercase names for constants. Header filenames for named types use PascalCase; feature headers use lowercase. Keep headers self-contained and include dependencies explicitly. Add brief Doxygen-style comments for public declarations, and follow existing include guards and nearby formatting.

## Testing

Add or update a focused `test/<module>/*_test.cpp` program for behavior changes; CMake recursively discovers `test/**/*.cpp` and registers each executable with CTest. Keep tests self-contained and use local fixtures or loopback networking. Socket tests use local ports 7777–7783, so ensure they are available. Run the affected executable or the full CTest suite before submitting.

## Commits & Pull Requests

Recent commits use a bracketed category, for example `[ADD]`, `[FIX]`, or `[TEST]`, followed by a concise summary; dependency updates may use the tool-generated subject. PRs should complete `.github/PULL_REQUEST_TEMPLATE.md`: explain motivation, link an issue or state “无”, report commands and results, describe compatibility impact, and check the relevant platform and style items.

## Dependencies & Platform Notes

CMake requires OpenSSL even for users who only consume non-network headers. CI covers Ubuntu/GCC and Windows/MSVC; consider both platforms for cross-platform changes.
