# shellcheck shell=bash
# Ubuntu row of the README tables. Sourced by smoke-package.sh.

prepare() {
  sudo apt-get update
  sudo apt-get install -y xvfb xauth
}

install_latest() {
  sudo add-apt-repository -y ppa:filesfm/worktime && sudo apt update && sudo apt install -y worktime
}

install_prelast() {
  sudo add-apt-repository -y ppa:filesfm/worktime && sudo apt update && sudo apt install -y "worktime=$PRELAST"
}

update() {
  sudo apt update && sudo apt upgrade -y worktime
}

remove() {
  sudo apt remove -y worktime && sudo add-apt-repository -y --remove ppa:filesfm/worktime
}

installed_version() {
  # shellcheck disable=SC2016 # dpkg-query format string, not a shell expansion
  dpkg-query -W -f='${Version}' worktime 2>/dev/null || true
}

is_installed() {
  # shellcheck disable=SC2016
  dpkg-query -W -f='${Status}' worktime 2>/dev/null | grep -q '^install ok installed$'
}

app_path() {
  echo /usr/bin/worktime
}
