# worktime

A Qt6/QML desktop time-tracking app. Uses a system tray icon when one is
available on the current desktop, falling back to a normal always-visible
window otherwise. Tracks time with a Start/Stop button and configures a
small settings table (server credentials, post interval, screenshot
capture options).

## Tech stack

- Qt 6.5+
- C++17
- CMake 3.16+
- QML for UI, C++ for business logic

## Documentation

The project has Doxygen support: `CMakeLists.txt` exposes a `docs` target
that generates HTML API docs from Doxygen comments in the source.

## Build & run

Two build types must be supported: **Debug** and **Release**. Always pass
`CMAKE_BUILD_TYPE` explicitly (single-config generators like Makefiles/Ninja
don't default to one) — don't leave it unset or assume Debug. Build trees
go under `build/Debug` and `build/Release`.

```
# Debug
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug

# Release
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```
