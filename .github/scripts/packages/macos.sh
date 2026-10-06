# shellcheck shell=bash
# macOS row of the README tables. Sourced by smoke-package.sh.
prepare() {
  :
}

install_latest() {
  brew install --cask filesfm/macos/worktime
}

install_prelast() {
  # Homebrew cannot pin a cask version and refuses casks outside a tap, so the tapped
  # cask file is replaced with the one from PRELAST_REF for this install, then restored.
  local tap
  tap=$(brew --repo filesfm/macos)
  gh api -H 'Accept: application/vnd.github.raw' \
    "repos/filesfm/homebrew-macos/contents/Casks/worktime.rb?ref=$PRELAST_REF" > "$tap/Casks/worktime.rb"
  HOMEBREW_NO_AUTO_UPDATE=1 brew install --cask filesfm/macos/worktime
  git -C "$tap" checkout -- Casks/worktime.rb
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
