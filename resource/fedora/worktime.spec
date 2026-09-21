%global debug_package %{nil}

Name:           worktime
Version:        __VERSION__
Release:        1
Summary:        A Qt6/QML desktop time-tracking app

License:        Apache-2.0
URL:            https://github.com/filesfm/WorkTime
Source0:        https://github.com/filesfm/WorkTime/archive/refs/tags/v%{version}.tar.gz

BuildRequires:  cmake >= 3.19
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  pkgconf-pkg-config
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  libX11-devel
BuildRequires:  systemd-devel
Requires:       hicolor-icon-theme

%description
WorkTime tracks working time with a simple Start/Stop button. It uses a
system tray icon when one is available on the current desktop, falling
back to a normal always-visible window otherwise).

%prep
%autosetup -n WorkTime-%{version}

%build
cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  -DCMAKE_INSTALL_PREFIX=%{_prefix}
cmake --build build/Release

%install
DESTDIR=%{buildroot} cmake --install build/Release

install -Dm644 resource/io.github.filesfm.worktime.desktop \
  %{buildroot}%{_datadir}/applications/io.github.filesfm.worktime.desktop
install -Dm644 resource/icon.svg \
  %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/io.github.filesfm.worktime.svg
install -Dm644 resource/icon.png \
  %{buildroot}%{_datadir}/icons/hicolor/128x128/apps/io.github.filesfm.worktime.png
install -Dm644 resource/io.github.filesfm.worktime.metainfo.xml \
  %{buildroot}%{_datadir}/metainfo/io.github.filesfm.worktime.metainfo.xml

%files
%license LICENSE
%{_bindir}/worktime
%{_datadir}/applications/io.github.filesfm.worktime.desktop
%{_datadir}/icons/hicolor/scalable/apps/io.github.filesfm.worktime.svg
%{_datadir}/icons/hicolor/128x128/apps/io.github.filesfm.worktime.png
%{_datadir}/metainfo/io.github.filesfm.worktime.metainfo.xml

%changelog
