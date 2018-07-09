Name:           lrexlib
Version:        2.9.0
Release:        1%{?dist}
Summary:        Regular expression handling library for Lua

Group:          Development/Libraries
License:        MIT
URL:            http://lrexlib.luaforge.net/
Source0:        lrexlib-%{version}.tar.gz

%description
Lrexlib are bindings of five regular expression library APIs (POSIX, PCRE and
GNU) to Lua.

%package pcre
Summary: Lua binding of PCRE library
BuildRequires:  pcre-devel
BuildRequires:  pkgconfig
BuildRequires:  tarantool-devel
BuildRequires:	luarocks
Requires:       tarantool >= 1.9.0.0
Provides:       lrexlib-pcre = %{version}

%description pcre
Binding of PCRE library

%package pcre2
Summary: Lua binding of PCRE-2 library
BuildRequires:  pcre2
BuildRequires:  pcre2-devel
BuildRequires:  pkgconfig
BuildRequires:  tarantool-devel
BuildRequires:	luarocks
Requires:       tarantool >= 1.9.0.0
Provides:       lrexlib-pcre2 = %{version}

%description pcre2
Binding of PCRE2 library

%package posix
Summary: Lua binding of POSIX library
BuildRequires:  pkgconfig
BuildRequires:  tarantool-devel
BuildRequires:	luarocks
Requires:       tarantool >= 1.9.0.0
Provides:       lrexlib-posix = %{version}

%description posix
Binding of POSIX library

%package gnu
Summary: Lua binding of GNU library
BuildRequires:  pkgconfig
BuildRequires:  tarantool-devel
BuildRequires:	luarocks
Requires:       tarantool >= 1.9.0.0
Provides:       lrexlib-gnu = %{version}

%description gnu
Binding of GNU library

%package oniguruma
Summary: Lua binding of Oniguruma library
BuildRequires:  oniguruma-devel
BuildRequires:  pkgconfig
BuildRequires:  tarantool-devel
BuildRequires:	luarocks
Requires:       tarantool >= 1.9.0.0
Provides:       lrexlib-oniguruma = %{version}

%description oniguruma
Binding of Oniguruma library

%package tre
Summary: Lua binding of TRE library
BuildRequires:  tre-devel
BuildRequires:  pkgconfig
BuildRequires:  tarantool-devel
BuildRequires:	luarocks
Requires:       tarantool >= 1.9.0.0
Provides:       lrexlib-tre = %{version}

%description tre
Binding of TRE library

%prep
%setup -q -n lrexlib-%{version}

for i in pcre pcre2 posix oniguruma tre gnu; do
    curl --remote-name http://luarocks.org/manifests/rrt/lrexlib-$i-2.9.0-1.src.rock
done

%build
mkdir tree
for i in pcre pcre2 posix oniguruma tre gnu; do
	TMP=$PWD/tmp luarocks --local --tree=./tree build lrexlib-$i-2.9.0-1.src.rock \
	CFLAGS="%{optflags} -fPIC -DLUA_COMPAT_APIINTCASTS -I/usr/include/tarantool" \
	PCRE_LIBDIR=%{_libdir}
done

%install
install -d %{buildroot}%{_datadir}

for i in pcre pcre2 posix tre gnu; do
    mkdir -p %{buildroot}%{_datadir}/tarantool/$i
    ls tree/%{_lib}/lua
    ls tree/%{_lib}/lua/5.1
    cp -P tree/%{_lib}/lua/5.1/rex_$i.so %{buildroot}%{_datadir}/tarantool/$i
done
mkdir -p %{buildroot}%{_datadir}/tarantool/oniguruma
cp -P tree/%{_lib}/lua/5.1/rex_onig.so %{buildroot}%{_datadir}/tarantool/oniguruma

%check
make %{?_smp_mflags} test

%files pcre
%dir %{_datadir}/tarantool/pcre
%{_datadir}/tarantool/pcre/

%files pcre2
%dir %{_datadir}/tarantool/pcre2
%{_datadir}/tarantool/pcre2/

%files posix
%dir %{_datadir}/tarantool/posix
%{_datadir}/tarantool/posix/

%files gnu
%dir %{_datadir}/tarantool/gnu
%{_datadir}/tarantool/gnu/

%files oniguruma
%dir %{_datadir}/tarantool/oniguruma
%{_datadir}/tarantool/oniguruma/

%files tre
%dir %{_datadir}/tarantool/tre
%{_datadir}/tarantool/tre/


%doc ChangeLog.old NEWS README.rst doc
%license LICENSE

%changelog
* Tue Aug 29 2017 Lubomir Rintel <lkundrak@v3.sk> - 2.8.0-1
- Update to a latest version

* Thu Aug 03 2017 Fedora Release Engineering <releng@fedoraproject.org> - 2.7.2-16
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Binutils_Mass_Rebuild

* Wed Jul 26 2017 Fedora Release Engineering <releng@fedoraproject.org> - 2.7.2-15
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Fri Feb 10 2017 Fedora Release Engineering <releng@fedoraproject.org> - 2.7.2-14
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Sun Oct 30 2016 Mamoru TASAKA <mtasaka@fedoraproject.org> - 2.7.2-13
- Rebuild for oniguruma 6.1.1

* Mon Jul 18 2016 Mamoru TASAKA <mtasaka@fedoraproject.org> - 2.7.2-12
- Rebuild for oniguruma 6

* Thu Feb 04 2016 Fedora Release Engineering <releng@fedoraproject.org> - 2.7.2-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Wed Jun 17 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.7.2-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Thu May 14 2015 Ville Skyttä <ville.skytta@iki.fi> - 2.7.2-9
- Mark LICENSE as %%license, don't ship .gitignore

* Thu Jan 15 2015 Tom Callaway <spot@fedoraproject.org> - 2.7.2-8
- rebuild for lua 5.3

* Sun Aug 17 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.7.2-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.7.2-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Thu Oct 24 2013 Lubomir Rintel <lkundrak@v3.sk> - 2.7.2-5
- Bulk sad and useless attempt at consistent SPEC file formatting

* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.7.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Tue Jun  4 2013 Tom Callaway <spot@fedoraproject.org>	- 2.7.2-3
- use lua(abi) for Requires. A B I.

* Mon Jun  3 2013 Tom Callaway <spot@fedoraproject.org> - 2.7.2-2
- use lua(api) for Requires

* Sun May 12 2013 Tom Callaway <spot@fedoraproject.org> - 2.7.2-1
- update to 2.7.2

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.4.0-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.4.0-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Fri Feb 10 2012 Petr Pisar <ppisar@redhat.com> - 2.4.0-8
- Rebuild against PCRE 8.30

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.4.0-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.4.0-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sat Jul 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.4.0-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.4.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Tue Dec 23 2008 Lubomir Rintel <lkundrak@v3.sk> - 2.4.0-3
- Compile shared library as PIC

* Wed Dec 17 2008 Lubomir Rintel <lkundrak@v3.sk> - 2.4.0-2
- Add doc directory to documentation
- Allow parallel make runs

* Tue Dec 16 2008 Lubomir Rintel <lkundrak@v3.sk> - 2.4.0-1
- Initial packaging
