#!/usr/bin/env bash
# Runs the README install lifecycle for one package row:
#   install latest -> run 20 s -> remove -> install pre-last -> run 20 s
#   -> update -> run 20 s -> remove.
# Usage: smoke-package.sh <key>
# Env from package_versions.py: LATEST, PRELAST, PRELAST_REF, DISCOVERY_ERROR.
# Writes ${RUNNER_TEMP}/status/<key>.json with passing, failing or skipped.
set -euo pipefail

key=$1
here=$(cd "$(dirname "$0")" && pwd)
status_file=${RUNNER_TEMP:-/tmp}/status/$key.json
mkdir -p "$(dirname "$status_file")"
skipped=false

finish() {
  local code=$?
  if [ "$skipped" = true ]; then
    printf '{"status": "skipped"}\n' > "$status_file"
  elif [ "$code" -eq 0 ]; then
    printf '{"status": "passing"}\n' > "$status_file"
  else
    printf '{"status": "failing"}\n' > "$status_file"
  fi
  exit "$code"
}
trap finish EXIT

fail() {
  echo "::error::$key: $*" >&2
  exit 1
}

step() {
  echo "==> $key: $*"
}

expect_version() {
  local got
  got=$(installed_version)
  if [ "$got" != "$1" ]; then
    fail "expected version $1 installed, found '${got:-none}'"
  fi
}

expect_removed() {
  if is_installed; then
    fail "package still installed after removal"
  fi
}

run_app() {
  step "run for 20 seconds"
  bash "$here/run-for-20s.sh" "$(app_path)"
}

if [ -n "${DISCOVERY_ERROR:-}" ]; then
  fail "version discovery failed: $DISCOVERY_ERROR"
fi

# Packages with no pre-last version are skipped for now, as a whole.
if [ -z "${PRELAST:-}" ]; then
  echo "No pre-last version is published for $key; skipping."
  skipped=true
  exit 0
fi

[ -n "${LATEST:-}" ] || fail "no latest version is published"

# shellcheck source=/dev/null
source "$here/packages/$key.sh"

workdir=${RUNNER_TEMP:-/tmp}/smoke-$key
mkdir -p "$workdir"
cd "$workdir"

step "prepare"
prepare

step "install latest $LATEST"
install_latest
expect_version "$LATEST"
run_app

step "remove"
remove
expect_removed

step "install pre-last $PRELAST"
install_prelast
expect_version "$PRELAST"
run_app

step "update to $LATEST"
update
expect_version "$LATEST"
run_app

step "remove"
remove
expect_removed

step "all checks passed"
