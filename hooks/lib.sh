#!/usr/bin/env bash
#
# Shared helpers for the clang-tidy/clang-format git hooks (pre-commit,
# pre-push). Must be sourced, not executed.

require_clang_tidy() {
    if ! command -v clang-tidy >/dev/null 2>&1; then
        echo "clang-tidy not found on PATH." >&2
        return 1
    fi
}

require_clang_format() {
    if ! command -v clang-format >/dev/null 2>&1; then
        echo "clang-format not found on PATH." >&2
        return 1
    fi
}

# Prints a usable build dir (one with compile_commands.json) on stdout.
find_build_dir() {
    local candidate
    for candidate in build/Debug build/Release build/*; do
        if [[ -f "$candidate/compile_commands.json" ]]; then
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
    if [[ "$#" -eq 0 ]]; then
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

# run_clang_format_check_on <file>...
# Diffs each file against its clang-format output, printing a diff for any
# file that isn't already clang-format-clean. Returns non-zero if any file
# needs reformatting.
run_clang_format_check_on() {
    if [[ "$#" -eq 0 ]]; then
        return 0
    fi

    local status=0
    local f
    for f in "$@"; do
        if ! diff -u --label "$f" --label "$f (clang-format)" "$f" <(clang-format "$f"); then
            status=1
        fi
    done

    return $status
}
