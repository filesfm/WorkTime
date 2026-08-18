# worktime

A Qt6/QML desktop time-tracking app.

## Contents

- [Build for development](#build-for-development)
- [Build for usage](#build-for-usage)

## Build for development

Requirements: Qt 6.5+, CMake 3.19+, a C++17 compiler, a CMake generator (e.g.
Ninja), and `clang-tidy` on `PATH` (used by the pre-commit/pre-push git
hooks, which CMake enables automatically on configure).

```
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/Debug
```

Run the test suite:

```
ctest --test-dir build/Debug --output-on-failure
```

Optionally build the Doxygen API docs (requires Doxygen installed):

```
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_DOCS=ON
cmake --build build/Debug --target docs
```

Output is written to `build/Debug/docs/html/index.html`.

## Build for usage

Just want to run the app? Configure and build a Release binary — no tests,
no docs:

```
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```

The built app is at `build/Release/bin/worktime` (`worktime.exe` on Windows,
`worktime.app` on macOS).
