# shellcheck shell=bash
# Fedora row of the README tables. Sourced by smoke-package.sh.

prepare() {
  # The fedora container has no sudo, copr plugin or X server.
  dnf install -y --setopt=install_weak_deps=False \
    sudo dnf-plugins-core xorg-x11-server-Xvfb xorg-x11-xauth
}

install_latest() {
  sudo dnf copr enable -y filesfm/WorkTime && sudo dnf install -y worktime
}

install_prelast() {
  sudo dnf copr enable -y filesfm/WorkTime && sudo dnf install -y "worktime-$PRELAST"
}

update() {
  sudo dnf upgrade -y worktime
}

remove() {
  sudo dnf remove -y worktime && sudo dnf copr disable -y filesfm/WorkTime
}

installed_version() {
  rpm -q --qf '%{VERSION}-%{RELEASE}' worktime 2>/dev/null || true
}

is_installed() {
  rpm -q worktime >/dev/null 2>&1
}

app_path() {
  echo /usr/bin/worktime
}
