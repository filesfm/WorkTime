# shellcheck shell=bash
# Windows row of the README tables. Sourced by smoke-package.sh (runs in Git Bash).

prepare() {
  :
}

install_latest() {
  choco install -y worktime
}

install_prelast() {
  choco install -y worktime --version "$PRELAST"
}

update() {
  choco upgrade -y worktime
}

remove() {
  choco uninstall -y worktime
}

installed_version() {
  choco list --local-only --exact --limit-output worktime 2>/dev/null | cut -d'|' -f2 || true
}

is_installed() {
  choco list --local-only --exact --limit-output worktime 2>/dev/null | grep -q '^worktime|'
}

app_path() {
  # The NSIS installer uses $PROGRAMFILES64\WorkTime (resource/windows/installer.nsi).
  # shellcheck disable=SC2154 # PROGRAMFILES is set by the Windows runner
  echo "$(cygpath -u "$PROGRAMFILES")/WorkTime/worktime.exe"
}
