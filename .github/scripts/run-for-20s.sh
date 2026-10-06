#!/usr/bin/env bash
# Starts an installed WorkTime binary, lets it run for 20 seconds and fails if it
# exited early or wrote a fatal error. Stopping it afterwards is expected, not an error.
# Usage: run-for-20s.sh <path-to-binary>
set -euo pipefail

app=$1
seconds=20
log=$(mktemp)

case "$(uname -s)" in
  MINGW* | MSYS* | CYGWIN*)
    powershell -NoProfile -ExecutionPolicy Bypass -File "$(dirname "$0")/run-for-20s.ps1" \
      -App "$(cygpath -w "$app")" -Seconds "$seconds" -Log "$(cygpath -w "$log")"
    exit 0
    ;;
esac

# Linux runners have no display: run under a virtual X server, as a real desktop would.
if [ "$(uname -s)" = Linux ] && [ -z "${DISPLAY:-}" ]; then
  exec xvfb-run -a bash "$0" "$@"
fi

"$app" > "$log" 2>&1 &
pid=$!
sleep "$seconds"

if ! kill -0 "$pid" 2>/dev/null; then
  status=0
  wait "$pid" || status=$?
  cat "$log"
  echo "::error::$app exited within $seconds seconds (exit code $status)"
  exit 1
fi

kill -TERM "$pid"
wait "$pid" 2>/dev/null || true
cat "$log"

if grep -E -q 'FATAL|Segmentation fault|core dumped|QCritical|Aborted' "$log"; then
  echo "::error::$app logged a fatal error"
  exit 1
fi
