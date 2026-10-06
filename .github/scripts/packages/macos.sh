# shellcheck shell=bash
# macOS row of the README tables. Sourced by smoke-package.sh.
prepare() {
  :
}

install_latest() {
  brew install --cask filesfm/macos/worktime
}

install_prelast() {
  # Homebrew cannot pin a cask version, so install the cask file as it was at PRELAST_REF.
  mkdir -p prelast
  gh api -H 'Accept: application/vnd.github.raw' \
    "repos/filesfm/homebrew-macos/contents/Casks/worktime.rb?ref=$PRELAST_REF" > prelast/worktime.rb
  brew install --cask "$PWD/prelast/worktime.rb"
}

update() {
  brew upgrade --cask filesfm/macos/worktime
}

remove() {
  brew uninstall --cask filesfm/macos/worktime
}

installed_version() {
  brew list --cask --versions worktime 2>/dev/null | awk '{print $2}' || true
}

is_installed() {
  brew list --cask worktime >/dev/null 2>&1
}

app_path() {
  local exe
  exe=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' /Applications/WorkTime.app/Contents/Info.plist)
  echo "/Applications/WorkTime.app/Contents/MacOS/$exe"
}
