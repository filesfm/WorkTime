# worktime

[![Test coverage](https://img.shields.io/codecov/c/github/filesfm/WorkTime?label=test%20coverage&style=for-the-badge&logo=codecov&logoColor=white)](https://codecov.io/gh/filesfm/WorkTime)
[![Lines of code](https://www.aschey.tech/tokei/github.com/filesfm/WorkTime?category=code&style=for-the-badge)](https://github.com/filesfm/WorkTime)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/quality_gate?project=filesfm_WorkTime)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=sqale_index)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=bugs)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=code_smells)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=duplicated_lines_density)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)

[![files-fm-worktime](https://snapcraft.io/files-fm-worktime/badge.svg)](https://snapcraft.io/files-fm-worktime)

A Qt6/QML desktop time-tracking app.

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

### macOS (works on x86-64 and ARM)

`brew install --cask filesfm/macos/worktime`

### Windows (works on x64)

#### Manually

- Download the latest version of the setup file from GitHub releases
- Run the installer
- Remove the installation file

### Fedora (Copr)

`sudo dnf copr enable -y filesfm/WorkTime && sudo dnf install -y worktime`

### Ubuntu (PPA)

`sudo add-apt-repository -y ppa:filesfm/worktime && sudo apt update && sudo apt install -y worktime`

### Arch

#### Manually

- Download the latest version of the package file(.pkg.tar.zst) from GitHub releases
- Enter the command and replace the *pathname* in it: `sudo pacman -U --noconfirm pathname`
- Remove the package file(.pkg.tar.zst)

---

## Updating

### macOS

`brew upgrade --cask filesfm/macos/worktime`

### Windows

#### Manually

- Download the latest version of the setup file from GitHub releases
- Go through the installation steps
- Remove the installation file

### Fedora (Copr)

`sudo dnf upgrade -y worktime`

### Ubuntu (PPA)

`sudo apt update && sudo apt upgrade -y worktime`

### Arch

#### Manually

- Download the latest version of the package file(.pkg.tar.zst) from GitHub releases
- Enter the command and replace the *pathname* in it: `sudo pacman -U --noconfirm pathname`
- Remove the package file(.pkg.tar.zst)

---

## Removing

### macOS

`brew uninstall --cask filesfm/macos/worktime`

### Windows

#### Manually

- Open the directory with the installed program
- Run the uninstall file

### Fedora (Copr)

`sudo dnf remove -y worktime && sudo dnf copr disable -y filesfm/WorkTime`

### Ubuntu (PPA)

`sudo apt remove -y worktime && sudo add-apt-repository -y --remove ppa:filesfm/worktime`

### Arch

#### Manually

`sudo pacman -Rs --noconfirm filesfm-worktime`
