Summary:  terminfo action checker
%global AppProgram tack
%global AppVersion 1.09
%global AppPatched 20240430
%global MySite https://invisible-island.net
# $XTermId: tack.spec,v 1.32 2024/04/30 07:40:32 tom Exp $
Name: %{AppProgram}
Version: %{AppVersion}
Release: %{AppPatched}
License: GPL2
Group: Applications/Development
URL: %{MySite}/ncurses/tack
Source0: %{MySite}/archives/ncurses/current/%{AppProgram}-%{AppVersion}-%{AppPatched}.tgz
Packager: Thomas Dickey <dickey@invisible-island.net>

%description
The 'tack' program is a diagnostic tool that is designed to create and verify
the correctness of terminfo's. This program can be used to create new terminal
descriptions that are not included in the standard ncurses release.

%prep

%define debug_package %{nil}

%setup -q -n %{AppProgram}-%{AppVersion}-%{AppPatched}

%build

INSTALL_PROGRAM='${INSTALL}' \
%configure \
		--target %{_target_platform} \
		--prefix=%{_prefix} \
		--bindir=%{_bindir} \
		--libdir=%{_libdir} \
		--mandir=%{_mandir} \
		--with-ncurses

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install DESTDIR=$RPM_BUILD_ROOT

strip $RPM_BUILD_ROOT%{_bindir}/%{AppProgram}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_prefix}/bin/%{AppProgram}
%{_mandir}/man1/%{AppProgram}.*

%changelog
# each patch should add its ChangeLog entries here

* Wed Jul 26 2017 Thomas Dickey
  use system-defined build-flags

* Sat Sep 04 2010 Thomas Dickey
- initial version
