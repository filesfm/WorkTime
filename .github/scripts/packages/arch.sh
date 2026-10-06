# shellcheck shell=bash
# Arch row of the README tables. Sourced by smoke-package.sh.
# Currently always skipped: deployment.yml keeps only the newest package in the repo.

prepare() {
  pacman -Sy --noconfirm sudo xorg-server-xvfb xorg-xauth
}

install_latest() {
  sudo sh -c 'grep -q "^\[worktime\]" /etc/pacman.conf || printf "\n[worktime]\nSigLevel = Optional TrustAll\nServer = https://filesfm.github.io/WorkTime/arch/\$arch\n" >> /etc/pacman.conf' && sudo pacman -Sy --noconfirm filesfm-worktime
}

install_prelast() {
  echo "::error::no pre-last package is available for Arch" >&2
  return 1
}

update() {
  sudo pacman -Sy --noconfirm filesfm-worktime
}

remove() {
  sudo pacman -Rs --noconfirm filesfm-worktime; sudo sed -i '/^\[worktime\]$/,+2d' /etc/pacman.conf
}

installed_version() {
  pacman -Q filesfm-worktime 2>/dev/null | awk '{print $2}' || true
}

is_installed() {
  pacman -Q filesfm-worktime >/dev/null 2>&1
}

app_path() {
  echo /usr/bin/worktime
}
