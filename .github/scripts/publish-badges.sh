#!/usr/bin/env bash
# Publishes the badge JSON to the package-status branch, which holds nothing but badges.
# It is separate from gh-pages, because the Doxygen deploy replaces gh-pages on every push to main.
# Env: VERSIONS (versions JSON), STATUS_DIR (downloaded status artifacts).
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
branch=package-status
out=${RUNNER_TEMP:-/tmp}/$branch

if git ls-remote --exit-code --heads origin "$branch" >/dev/null 2>&1; then
  git fetch --depth=1 origin "$branch"
  git worktree add "$out" -B "$branch" FETCH_HEAD
else
  git worktree add --orphan -b "$branch" "$out"
fi

mkdir -p "$out/badges"
printf '%s' "$VERSIONS" | python3 "$here/make_badges.py" "$STATUS_DIR" "$out/badges"

cd "$out"
git add badges
if git diff --cached --quiet; then
  echo "Badges unchanged."
  exit 0
fi

git -c user.name='github-actions[bot]' \
  -c user.email='41898282+github-actions[bot]@users.noreply.github.com' \
  commit -m "chore(badges): update package status"
git push origin "HEAD:$branch"
