# shellcheck shell=bash
# Linux (General) row of the README tables: the AppImage. Sourced by smoke-package.sh.
# The README command downloads the newest AppImage into the current directory.
# Removal and the pre-last download are not in the README, so they are defined here.

prepare() {
  sudo apt-get update
  sudo apt-get install -y xvfb xauth libopengl0 libegl1 libxkbcommon0 libxkbcommon-x11-0
  # AppImages need FUSE 2; the package name differs by Ubuntu release.
  sudo apt-get install -y libfuse2t64 || sudo apt-get install -y libfuse2
}

download_latest() {
  curl -sL https://api.github.com/repos/filesfm/WorkTime/releases/latest \
    | grep -o '"browser_download_url": *"[^"]*\.AppImage"' \
    | head -n1 | cut -d'"' -f4 | xargs -r curl -fL -O
  chmod +x worktime-v*-x86_64.AppImage
}

install_latest() {
  download_latest
}

install_prelast() {
  gh release download "v$PRELAST" -R filesfm/WorkTime --pattern "worktime-v$PRELAST-x86_64.AppImage"
  chmod +x "worktime-v$PRELAST-x86_64.AppImage"
}

update() {
  download_latest
}

remove() {
  rm -f worktime-v*-x86_64.AppImage
}

# Update keeps the older AppImage next to the new one, so report the highest version present.
installed_version() {
  local file versions=()
  for file in worktime-v*-x86_64.AppImage; do
    [ -e "$file" ] || continue
    file=${file#worktime-v}
    versions+=("${file%-x86_64.AppImage}")
  done
  printf '%s\n' "${versions[@]}" | sort -V | tail -n1
}

is_installed() {
  compgen -G 'worktime-v*-x86_64.AppImage' >/dev/null
}

app_path() {
  echo "$PWD/worktime-v$(installed_version)-x86_64.AppImage"
}
