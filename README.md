# WorkTime

## SonarQube

[![Quality gate status](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=coverage)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=sqale_index)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=bugs)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=code_smells)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)
[![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=filesfm_WorkTime&metric=duplicated_lines_density)](https://sonarcloud.io/summary/new_code?id=filesfm_WorkTime)

WorkTime is a desktop app for tracking how you spend your work time.

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

### Arch

#### Manually

- Download the latest version of the package file(.pkg.tar.zst) from GitHub releases
- Enter the command and replace the *pathname* in it: `sudo pacman -U --noconfirm pathname`
- Remove the package file(.pkg.tar.zst)

---

## Updating

| Operating system | Terminal command                                  |
| ---------------- | ------------------------------------------------- |
| macOS            | `brew upgrade --cask filesfm/macos/worktime`      |
| Windows          | `choco upgrade -y worktime`                       |
| Fedora           | `sudo dnf upgrade -y worktime`                    |
| Ubuntu           | `sudo apt update && sudo apt upgrade -y worktime` |

### Arch

#### Manually

- Download the latest version of the package file(.pkg.tar.zst) from GitHub releases
- Enter the command and replace the *pathname* in it: `sudo pacman -U --noconfirm pathname`
- Remove the package file(.pkg.tar.zst)

---

## Removing

| Operating system | Terminal command                                                                          |
| ---------------- | ----------------------------------------------------------------------------------------- |
| macOS            | `brew uninstall --cask filesfm/macos/worktime`                                            |
| Windows          | `choco uninstall -y worktime`                                                             |
| Fedora           | `sudo dnf remove -y worktime && sudo dnf copr disable -y filesfm/WorkTime`                |
| Ubuntu           | `sudo apt remove -y worktime && sudo add-apt-repository -y --remove ppa:filesfm/worktime` |

### Arch

#### Manually

`sudo pacman -Rs --noconfirm filesfm-worktime`
