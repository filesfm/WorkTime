# WorkTime

WorkTime is a desktop app for tracking how you spend your work time.

## SonarQube

[![Quality gate status](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=coverage)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)

## Package status

| Package         | Latest version                                                                                                                                                                                                                                      | Test status                                                                                                                                                                                                                                                       |
| --------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| macOS           | ![macOS version](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Fmacos-version.json)                                                                                  | [![macOS test](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Fmacos-status.json)](https://github.com/filesfm/WorkTime/actions/workflows/package-smoke.yml)                     |
| Windows         | ![Windows version](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Fwindows-version.json)                                                                              | [![Windows test](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Fwindows-status.json)](https://github.com/filesfm/WorkTime/actions/workflows/package-smoke.yml)                 |
| Fedora          | ![Fedora version](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Ffedora-version.json)                                                                                | [![Fedora test](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Ffedora-status.json)](https://github.com/filesfm/WorkTime/actions/workflows/package-smoke.yml)                   |
| Ubuntu          | ![Ubuntu version](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Fubuntu-version.json)                                                                                | [![Ubuntu test](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Fubuntu-status.json)](https://github.com/filesfm/WorkTime/actions/workflows/package-smoke.yml)                   |
| Arch            | ![Arch version](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Farch-version.json)                                                                                      | [![Arch test](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Farch-status.json)](https://github.com/filesfm/WorkTime/actions/workflows/package-smoke.yml)                       |
| Linux (General) | ![Linux version](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Flinux-version.json)                                                                                    | [![Linux test](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Ffilesfm%2FWorkTime%2Fpackage-status%2Fbadges%2Flinux-status.json)](https://github.com/filesfm/WorkTime/actions/workflows/package-smoke.yml)                   |

## Contents

- [Supported Operating Systems](#supported-operating-systems)
- [Supported Desktop Environments](#supported-desktop-environments-on-linux)
- [Installing](#installing)
- [Updating](#updating)
- [Removing](#removing)

---

## Supported operating systems

- macOS
- Windows
- Linux

## Supported desktop environments on Linux

- GNOME
- KDE Plasma
- Cinnamon

---

## Installing

| Operating system | Terminal command                                                                                     |
| ---------------- | ---------------------------------------------------------------------------------------------------- |
| macOS            | `brew install --cask filesfm/macos/worktime`                                                         |
| Windows          | `choco install -y worktime`                                                                          |
| Fedora           | `sudo dnf copr enable -y filesfm/WorkTime && sudo dnf install -y worktime`                           |
| Ubuntu           | `sudo add-apt-repository -y ppa:filesfm/worktime && sudo apt update && sudo apt install -y worktime` |
| Arch             | `sudo sh -c 'grep -q "^\[worktime\]" /etc/pacman.conf \|\| printf "\n[worktime]\nSigLevel = Optional TrustAll\nServer = https://filesfm.github.io/WorkTime/arch/\$arch\n" >> /etc/pacman.conf' && sudo pacman -Sy --noconfirm filesfm-worktime` |
| Linux (General)  | `curl -sL https://api.github.com/repos/filesfm/WorkTime/releases/latest \| grep -o '"browser_download_url": *"[^"]*\.AppImage"' \| head -n1 \| cut -d'"' -f4 \| xargs -r curl -fL -O` |

---

## Updating

| Operating system | Terminal command                                  |
| ---------------- | ------------------------------------------------- |
| macOS            | `brew upgrade --cask filesfm/macos/worktime`      |
| Windows          | `choco upgrade -y worktime`                       |
| Fedora           | `sudo dnf upgrade -y worktime`                    |
| Ubuntu           | `sudo apt update && sudo apt upgrade -y worktime` |
| Arch             | `sudo pacman -Sy --noconfirm filesfm-worktime`    |
| Linux (General)  | `curl -sL https://api.github.com/repos/filesfm/WorkTime/releases/latest \| grep -o '"browser_download_url": *"[^"]*\.AppImage"' \| head -n1 \| cut -d'"' -f4 \| xargs -r curl -fL -O` |

---

## Removing

| Operating system | Terminal command                                                                                    |
| ---------------- | --------------------------------------------------------------------------------------------------- |
| macOS            | `brew uninstall --cask filesfm/macos/worktime`                                                      |
| Windows          | `choco uninstall -y worktime`                                                                       |
| Fedora           | `sudo dnf remove -y worktime && sudo dnf copr disable -y filesfm/WorkTime`                          |
| Ubuntu           | `sudo apt remove -y worktime && sudo add-apt-repository -y --remove ppa:filesfm/worktime`           |
| Arch             | `sudo pacman -Rs --noconfirm filesfm-worktime; sudo sed -i '/^\[worktime\]$/,+2d' /etc/pacman.conf` |
