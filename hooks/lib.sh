#!/usr/bin/env bash
#
# Shared helpers for the clang-tidy git hooks (pre-commit, pre-push).
# Must be sourced, not executed.

require_clang_tidy() {
    if ! command -v clang-tidy >/dev/null 2>&1; then
        echo "clang-tidy not found on PATH." >&2
        return 1
    fi
}

# Prints a usable build dir (one with compile_commands.json) on stdout.
find_build_dir() {
    local candidate
    for candidate in build/Debug build/Release build/*; do
        if [ -f "$candidate/compile_commands.json" ]; then
            echo "$candidate"
            return 0
        fi
    done

    return 1
}

print_no_build_dir_help() {
    echo "No usable build directory found under build/*." >&2
    echo "Configure one first, e.g.:" >&2
    echo "  cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" >&2
    echo "  cmake --build build/Debug" >&2
}

# run_clang_tidy_on <build_dir> <file>...
run_clang_tidy_on() {
    local build_dir=$1
    shift
    if [ "$#" -eq 0 ]; then
        return 0
    fi

    local jobs
    if command -v nproc >/dev/null 2>&1; then
        jobs=$(nproc)
    else
        jobs=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi

    printf '%s\0' "$@" | xargs -0 -P "$jobs" -n1 clang-tidy -p "$build_dir"
}
