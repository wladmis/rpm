# python-module-rpm overrides %%version
%define rpm_version 4.0.4

Name: rpm
Version: 4.0.4
Release: alt99.M51.2

%define ifdef() %if %{expand:%%{?%{1}:1}%%{!?%{1}:0}}
%define get_dep() %(rpm -q --qf '%%{NAME} >= %%|SERIAL?{%%{SERIAL}:}|%%{VERSION}-%%{RELEASE}' %1 2>/dev/null || echo '%1 >= unknown')
%define def_with() %{expand:%%{!?_with_%{1}: %%{!?_without_%{1}: %%global _with_%{1} --with-%{1}}}}
%define def_without() %{expand:%%{!?_with_%{1}: %%{!?_without_%{1}: %%global _without_%{1} --without-%{1}}}}
%define if_with() %if %{expand:%%{?_with_%{1}:1}%%{!?_with_%{1}:0}}
%define if_without() %if %{expand:%%{?_without_%{1}:1}%%{!?_without_%{1}:0}}
%define _rpmlibdir %_prefix/lib/rpm

%def_with python
%def_with libelf
%def_without apidocs
%def_without db
%def_without build_topdir
%def_without selinux

# XXX enable at your own risk, CDB access to rpmdb isn't cooked yet.
%define enable_cdb create cdb

Summary: The RPM package management system
Summary(ru_RU.UTF-8): Менеджер пакетов RPM
License: GPL
Group: System/Configuration/Packaging
Url: http://www.rpm.org/
Packager: Dmitry V. Levin <ldv@altlinux.org>

# http://git.altlinux.org/people/ldv/packages/?p=rpm.git
Source: rpm-%version-%release.tar

Provides: %_rpmlibdir/macros.d, %_sysconfdir/%name/macros.d

PreReq: lib%name = %version-%release, librpmbuild = %version-%release
PreReq: alt-gpgkeys, coreutils, /bin/sh

# Due to rpm*cmp utilities.
Conflicts: rpm-utils <= 0:0.9.10-alt1

%{?_with_python:BuildPreReq: python-devel}
%{?_with_apidocs:BuildPreReq: ctags doxygen}
%{?_with_libelf:BuildPreReq: libelf-devel-static}
%{?_with_selinux:BuildPreReq: libselinux-devel-static >= 2.0.96}

BuildPreReq: automake >= 1.7.1, autoconf >= 2.53, libbeecrypt-devel-static >= 4.2.1,
BuildPreReq: rpm >= 3.0.6-ipl24mdk, %_bindir/subst

# Automatically added by buildreq on Thu Apr 23 2009 and edited manually.
BuildRequires: bzlib-devel-static libdb4.7-devel-static libelf-devel-static liblzma-devel-static libpopt-devel-static python-devel zlib-devel-static

%package -n lib%name
Summary: Shared libraries required for applications which will manipulate RPM packages
Summary(ru_RU.UTF-8): Файлы, необходимые для разработки приложений, взаимодействующих с RPM-пакетами
License: GPL/LGPL
Group: System/Libraries
PreReq: zlib >= 1.1.4
PreReq: bzlib >= 1:1.0.2-alt2
PreReq: libpopt >= 1:1.7-alt3
PreReq: libdb4.7

%package -n librpmbuild
Summary: Shared library required for applications which will build RPM packages
Summary(ru_RU.UTF-8): Разделяемая библиотека для разработки приложений, собирающих RPM-пакеты
License: GPL/LGPL
Group: System/Libraries
Requires: lib%name = %version-%release

%package -n lib%name-devel
Summary: Development files for applications which will manipulate RPM packages
Summary(ru_RU.UTF-8): Файлы, необходимые для разработки приложений, взаимодействующих с RPM-пакетами
License: GPL/LGPL
Group: Development/C
Provides: %name-devel = %version-%release
Obsoletes: %name-devel
Requires: lib%name = %version-%release, librpmbuild = %version-%release
Requires: libpopt-devel

%package -n lib%name-devel-static
Summary: Static libraries for developing statically linked applications which will manipulate RPM packages
Summary(ru_RU.UTF-8): Статические библиотеки, необходимые для разработки статических приложений, взаимодействующих с RPM-пакетами
License: GPL/LGPL
Group: Development/C
Requires: lib%name-devel = %version-%release
Requires: bzlib-devel-static, libbeecrypt-devel-static, libdb4.7-devel-static, libpopt-devel-static, zlib-devel-static

%package build
Summary: Scripts and executable programs used to build packages
Summary(ru_RU.UTF-8): Файлы, необходимые для установки SRPM-пакетов и сборки RPM-пакетов
License: GPL
Group: Development/Other
Obsoletes: spec-helper
PreReq: librpmbuild = %version-%release, %name = %version-%release
PreReq: shadow-utils
Requires: autoconf autoconf-common automake automake-common bison coreutils cpio
Requires: gcc gettext-tools glibc-devel file kernel-headers libtool m4 make
Requires: procps psmisc sed service texinfo which
Requires: bash >= 0:3.1.17-alt4
Requires: bzip2 >= 1:1.0.2-alt4
Requires: gzip >= 0:1.3.3-alt2
Requires: mktemp >= 1:1.3.1
Requires: patch >= 2.5
Requires: tar >= 0:1.13.22-alt1
Requires: %_bindir/subst
Requires: alternatives >= 0.3.2
Requires: elfutils >= 0.143-alt1
Requires: info-install >= 4.11
Requires: pkgconfig-reqprov pkgconfig-recursion
Requires: rpm-build-perl >= 0.6.2
Requires: rpm-build-python >= 0.31
Conflicts: rpm-build-tcl <= 0.2
Conflicts: rpm-build-mono <= 1.0
# Due to 'readelf --dyn-syms':
#Requires: binutils >= 1:2.20.51.0.7

%package build-topdir
Summary: RPM package installation and build directory tree
Summary(ru_RU.UTF-8): Сборочное дерево, используемое для установки SRPM-пакетов и сборки RPM-пакетов
License: GPL
Group: Development/Other
PreReq: %name-build = %version-%release

%package static
Summary: Static version of the RPM package management system
Summary(ru_RU.UTF-8): Статическая версия менеджера пакетов RPM
License: GPL
Group: System/Configuration/Packaging
PreReq: %name = %version-%release

%description
The RPM Package Manager (RPM) is a powerful command line driven
package management system capable of installing, uninstalling,
verifying, querying, and updating software packages.  Each software
package consists of an archive of files along with information about
the package like its version, a description, etc.

%description -l ru_RU.UTF-8
RPM - это мощный неинтерактивный менеджер пакетов, используемый для сборки,
установки, инспекции, проверки, обновления и удаления отдельных программных
пакетов.  Каждый такой пакет состоит из набора файлов и информации о пакете,
включающей название, версию, описание пакета, и т.д.

%description -n lib%name
This package contains shared libraries required to run dynamically linked
programs manipulating with RPM packages and databases.

%description -n librpmbuild
This package contains shared library required to run dynamically linked
programs building RPM packages.

%description -n lib%name-devel
This package contains the RPM C library and header files.  These
development files will simplify the process of writing programs
which manipulate RPM packages and databases and are intended to make
it easier to create graphical package managers or any other tools
that need an intimate knowledge of RPM packages in order to function.

This package should be installed if you want to develop programs that
will manipulate RPM packages and databases.

%description -n lib%name-devel-static
This package contains the RPM C library and header files.  These
development files will simplify the process of writing programs
which manipulate RPM packages and databases and are intended to make
it easier to create graphical package managers or any other tools
that need an intimate knowledge of RPM packages in order to function.

This package should be installed if you want to develop statically linked
programs that will manipulate RPM packages and databases.

%description build
This package contains scripts and executable programs that are used to
build packages using RPM.

%description build-topdir
This package contains RPM package installation and build directory tree.

%description static
This package contains statically linked version of the RPM program.

%if_with python
%package -n python-module-rpm
Version: %{rpm_version}_%__python_version
Summary: Python bindings for apps which will manipulate RPM packages
Summary(ru_RU.UTF-8): Интерфейс для разработки Python-приложений, взаимодействующих с RPM-пакетами
License: GPL/LGPL
Group: Development/Python
PreReq: lib%name = %rpm_version-%release
Requires: python = %__python_version
Provides: rpm-python = %{rpm_version}_%__python_version-%release
Obsoletes: rpm-python

%description -n python-module-rpm
This package contains a module which permits applications written in
the Python programming language to use the interface supplied by RPM
(RPM Package Manager) libraries.
%endif #with python

%prep
%setup -q -n rpm-%rpm_version-%release

%build
touch config.rpath
gettextize --force --quiet --no-changelog --symlink
install -pv -m644 /usr/share/automake/mkinstalldirs .
install -pv -m644 /usr/share/gettext/intl/Makevars* po/Makevars
autoreconf -fisv -I m4
# avoid extra build dependencies
export ac_cv_path___CPIO=/bin/cpio
export ac_cv_path___UNZIP=/usr/bin/unzip
export ac_cv_path___LZMA=/usr/bin/lzma
export ac_cv_path___XZ=/usr/bin/xz
export ac_cv_path___GPG=/usr/bin/gpg
export ac_cv_path___SSH=/usr/bin/ssh
%configure \
	%{?_with_python} %{?_without_python} \
	%{?_with_apidocs} %{?_without_apidocs} \
	%{?_with_db} %{?_without_db} \
	%{subst_with selinux} \
	--program-transform-name=

%{!?_enable_debug:%make_build -C lib set.lo CFLAGS='%optflags -O3'}
%make_build YACC='bison -y'
%if_with apidocs
rm -rf apidocs
make apidocs
%endif #with apidocs

%install
%make_install DESTDIR='%buildroot' install
chmod a-w %buildroot%_usrsrc/RPM{,/RPMS/*}

# Save list of packages through cron.
#mkdir -p %buildroot%_sysconfdir/cron.daily
#install -p -m750 scripts/%name.daily %buildroot%_sysconfdir/cron.daily/%name
#
#mkdir -p %buildroot%_sysconfdir/logrotate.d
#install -p -m640 scripts/%name.log %buildroot%_sysconfdir/logrotate.d/%name

mkdir -p %buildroot{%_rpmlibdir/macros.d,%_sysconfdir/%name/macros.d}
touch %buildroot%_sysconfdir/%name/macros
cat << E_O_F > %buildroot%_sysconfdir/%name/macros.cdb
%{?enable_cdb:#%%__dbi_cdb	%enable_cdb}
E_O_F

mkdir -p %buildroot%_localstatedir/%name
for dbi in \
	Basenames Conflictname Dirnames Group Installtid Name Providename \
	Provideversion Removetid Requirename Requireversion Triggername \
	Sigmd5 Sha1header Filemd5s Packages \
	__db.001 __db.002 __db.003 __db.004 __db.005 __db.006 __db.007 \
	__db.008 __db.009
do
    touch "%buildroot%_localstatedir/%name/$dbi"
done
touch %buildroot%_localstatedir/%name/files-awaiting-filetriggers

# Prepare documentation.
bzip2 -9k CHANGES ||:
mkdir -p %buildroot%_docdir/%name-%rpm_version
install -p -m644 CHANGES.bz2 CREDITS README README.ALT* \
	%buildroot%_docdir/%name-%rpm_version/
cp -a doc/manual %buildroot%_docdir/%name-%rpm_version/
rm -f %buildroot%_docdir/%name-%rpm_version/manual/{Makefile*,buildroot}
%if_with apidocs
cp -a apidocs/man/man3 %buildroot%_mandir/
cp -a apidocs/html %buildroot%_docdir/%name-%rpm_version/apidocs/
%endif #with apidocs

# rpminit(1).
install -pD -m755 rpminit %buildroot%_bindir/rpminit
install -pD -m644 rpminit.1 %buildroot%_man1dir/rpminit.1

# Valid groups.
install -p -m644 GROUPS %buildroot%_rpmlibdir/

# buildreq ignore rules.
install -pD -m644 rpm-build.buildreq %buildroot%_sysconfdir/buildreqs/files/ignore.d/rpm-build

chmod a+x scripts/find-lang
# Manpages have been moved to their own packages.
#./scripts/find-lang --with-man %name rpm2cpio --output %name.lang
RPMCONFIGDIR=./scripts ./scripts/find-lang %name rpm2cpio --output %name.lang

pushd %buildroot%_rpmlibdir
	for f in *-alt-%_target_os; do
		n=`echo "$f" |sed -e 's/-alt//'`
		[ -e "$n" ] || ln -s "$f" "$n"
	done
popd

/bin/ls -1d %buildroot%_rpmlibdir/*-%_target_os |
	grep -Fv /brp- |
	sed -e "s|^%buildroot|%%attr(-,root,%name) |g" >>%name.lang

%ifdef add_findreq_skiplist
# These shell libraries hopefully do not require anything special,
# but we want to keep the "rpm" package dependencies to bare minimum.
%add_findreq_skiplist %_rpmlibdir/functions
%add_findreq_skiplist %_rpmlibdir/find-package
%endif
# However, syntax check is still a good idea.
sh -n %buildroot%_rpmlibdir/functions
sh -n %buildroot%_rpmlibdir/find-package

%pre
[ ! -L %_rpmlibdir/noarch-alt-%_target_os ] || rm -f %_rpmlibdir/noarch-alt-%_target_os ||:

%post
chgrp %name %_localstatedir/%name/[A-Z]*
[ -n "$DURING_INSTALL" -o -n "$BTE_INSTALL" ] ||
	%_rpmlibdir/pdeath_execute $PPID rpmdb -v --rebuilddb

# Invalidate apt cache, due to e.g. rpmlib(PayloadIsLzma).
if set /var/cache/apt/*.bin && [ -f "$1" ]; then
	%_rpmlibdir/pdeath_execute $PPID rm -f "$@"
fi
:

%files -n librpmbuild
%_libdir/librpmbuild-*.so

%define rpmattr %attr(755,root,%name)
%define rpmdirattr %attr(2755,root,%name) %dir
%define rpmdatattr %attr(644,root,%name)
%define rpmdbattr %attr(644,root,%name) %verify(not md5 size mtime) %ghost %config(missingok,noreplace)

%files -n lib%name
%rpmdirattr %_rpmlibdir
%rpmdatattr %_rpmlibdir/rpmrc
%rpmdatattr %_rpmlibdir/macros
%_libdir/librpm-*.so
%_libdir/librpmdb-*.so
%_libdir/librpmio-*.so

%files -n lib%name-devel
%_libdir/librpm.so
%_libdir/librpmdb.so
%_libdir/librpmio.so
%_libdir/librpmbuild.so
%_includedir/%name
%if_with apidocs
%_man3dir/*
%dir %_docdir/%name-%rpm_version
%_docdir/%name-%rpm_version/apidocs
%endif #with apidocs

%files -n lib%name-devel-static
%_libdir/*.a

%files -f %name.lang
%dir %_docdir/%name-%rpm_version
%_docdir/%name-%rpm_version/[A-Z]*
%_docdir/%name-%rpm_version/manual

#%config(noreplace,missingok) %_sysconfdir/cron.daily/%name
#%config(noreplace,missingok) %_sysconfdir/logrotate.d/%name

%rpmdirattr %_rpmlibdir/macros.d
%dir %_sysconfdir/%name
%dir %_sysconfdir/%name/macros.d
%config(noreplace,missingok) %_sysconfdir/%name/macros
%config(noreplace,missingok) %_sysconfdir/%name/macros.??*

%rpmdirattr %_localstatedir/%name
%rpmdbattr %_localstatedir/%name/Basenames
%rpmdbattr %_localstatedir/%name/Conflictname
%rpmdbattr %_localstatedir/%name/__db.0*
%rpmdbattr %_localstatedir/%name/Dirnames
%rpmdbattr %_localstatedir/%name/Filemd5s
%rpmdbattr %_localstatedir/%name/Group
%rpmdbattr %_localstatedir/%name/Installtid
%rpmdbattr %_localstatedir/%name/Name
%rpmdbattr %_localstatedir/%name/Packages
%rpmdbattr %_localstatedir/%name/Providename
%rpmdbattr %_localstatedir/%name/Provideversion
%rpmdbattr %_localstatedir/%name/Removetid
%rpmdbattr %_localstatedir/%name/Requirename
%rpmdbattr %_localstatedir/%name/Requireversion
%rpmdbattr %_localstatedir/%name/Sigmd5
%rpmdbattr %_localstatedir/%name/Sha1header
%rpmdbattr %_localstatedir/%name/Triggername
%rpmdbattr %_localstatedir/%name/files-awaiting-filetriggers

/bin/rpm
%_bindir/rpm
%_bindir/rpm2cpio
%_bindir/rpmdb
%_bindir/rpm[eiu]
%_bindir/rpmsign
%_bindir/rpmquery
%_bindir/rpmverify
%_bindir/rpminit
%_bindir/rpm*cmp

%rpmattr %_rpmlibdir/pdeath_execute
%rpmattr %_rpmlibdir/rpm[dikq]
%_rpmlibdir/rpm[euv]
%rpmdatattr %_rpmlibdir/rpmpopt*
%rpmdatattr %_rpmlibdir/GROUPS
%_prefix/lib/rpmpopt
%_prefix/lib/rpmrc

%rpmattr %_rpmlibdir/posttrans-filetriggers
%rpmattr %_rpmlibdir/0ldconfig.filetrigger

%rpmattr %_rpmlibdir/functions
%rpmattr %_rpmlibdir/find-package
%rpmdatattr %_rpmlibdir/.provides.sh

%_man1dir/rpminit.*
%_man8dir/rpm.*
%_man8dir/rpm2cpio.*

%files build
%config %_sysconfdir/buildreqs/files/ignore.d/*
%rpmattr %_bindir/gendiff
%_bindir/rpmbuild
%_bindir/relative
%_rpmlibdir/rpmt
%rpmattr %_rpmlibdir/rpmb
%rpmattr %_rpmlibdir/dump_ld_config
%rpmattr %_rpmlibdir/filesize
%rpmattr %_rpmlibdir/relative
%rpmattr %_rpmlibdir/brp-*
%rpmattr %_rpmlibdir/*_files
%rpmattr %_rpmlibdir/check-files
%rpmattr %_rpmlibdir/ldd
%rpmattr %_rpmlibdir/rpm2cpio.sh
%rpmattr %_rpmlibdir/find-lang
%rpmattr %_rpmlibdir/find-provides
%rpmattr %_rpmlibdir/find-requires
%rpmattr %_rpmlibdir/find-scriptlet-requires
%rpmattr %_rpmlibdir/fixup-*
%rpmattr %_rpmlibdir/files.*
%rpmattr %config %_rpmlibdir/*-files.req.list
%rpmattr %_rpmlibdir/lib.*
%rpmattr %_rpmlibdir/pam.*
%rpmattr %_rpmlibdir/pkgconfig.*
%rpmattr %_rpmlibdir/pkgconfiglib.*
%rpmattr %_rpmlibdir/rpmlib.*
%rpmattr %_rpmlibdir/shell.*
%rpmattr %_rpmlibdir/shebang.*
%rpmattr %_rpmlibdir/static.*
%rpmattr %_rpmlibdir/symlinks.*
%rpmattr %_rpmlibdir/verify-elf
%rpmattr %_rpmlibdir/*.awk
# set-version helpers
%rpmattr %_rpmlibdir/mkset
%rpmattr %_rpmlibdir/setcmp

%_mandir/man?/gendiff.*
%_man8dir/rpmbuild.*

%if_with build_topdir
%files build-topdir
%attr(0755,root,%name) %dir %_usrsrc/RPM
%attr(0770,root,%name) %dir %_usrsrc/RPM/BUILD
%attr(2770,root,%name) %dir %_usrsrc/RPM/SPECS
%attr(2770,root,%name) %dir %_usrsrc/RPM/SOURCES
%attr(2775,root,%name) %dir %_usrsrc/RPM/SRPMS
%attr(0755,root,%name) %dir %_usrsrc/RPM/RPMS
%attr(2775,root,%name) %dir %_usrsrc/RPM/RPMS/*
%endif #with build_topdir

%if_with python
%files -n python-module-rpm
%_libdir/python*/site-packages/*module.so
%endif #with python

%files static
%_bindir/rpm.static
%_bindir/rpm2cpio.static

%changelog
* Tue Nov 23 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt99.M51.2
- Compiled set.c with -O3.

* Wed Oct 20 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt99.M51.1
- lib.req, lib.prov: Disabled soname set-versions (Alexey Tourbin).
- Disabled SELinux support.
- Built for branch 5.1.

* Wed Oct 20 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt100.1
- Changed release numeration:
  alt99.M51 series is for branch 5.1, alt100 is for Sisyphus (Alexey Tourbin).
- rpmrc: Updated for ARM (Mihail Yakushin).

* Mon Oct 04 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.49
- lib.req: Implemented set-versions for soname dependencies.
- build/parseReqs.c: Enabled dependencies on rpmlib(SetVersions).

* Fri Sep 24 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.48
- 0common-files.req.list: Added /etc/sysctl.d, /lib/udev/rules.d,
  /lib64/udev and /lib64/udev/rules.d entries.

* Mon Sep 20 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.47
- set.c (rpmsetcmp): Fixed check for set2 decoding error.
- brp-cleanup: Updated for /usr/lib64/perl5 and /usr/share/perl5.

* Sat Sep 11 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.46
- set.c: Implemented base62, golomb, and set-string routines.
- depends.c (rpmRangesOverlap): Added support for set-versions.
- rpmlibprov.c: Added rpmlib(SetVersions) feature.
- %_rpmlibdir/mkset: Command-line helper for making set-versions.
- lib.prov: Implemented soname set-versioning with exported symbols.

* Wed Aug 25 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.45
- Added SELinux support (by Mikhail Efremov and me).

* Sun Aug 22 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.44
- depends.c: Permit self-conflicting packages.
- verify.c: Updated verifyDependencies() for self-conflicting packages.

* Wed Aug 18 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.43
- depends.c: A better solution to dbProvCache dangling pointers without
  resorting to strdup (ALT#23811).

* Tue Aug 17 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.42
- Added .gitignore file, improved support for in-tree building.
- Removed %%__ccache* macros (rpm will use default ccache settings).

* Fri Aug 13 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.41
- Migrated to beecrypt-4.2.1 (by Kirill A. Shutemov).

* Thu Aug 05 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.40
- build/files.c (parseForSimple): Fix potential NULL pointer dereference
  (Dmitry V. Levin, ALT#23813).
- depends.c (dbSatisfiesDepend): Use strdup for dbProvCache keys
  to avoid dangling pointers (ALT#23811).

* Mon Jul 12 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.39
- depends.c: Introduced ts->erasedPackages list of headers.
- depends.c: Replaced dbi-based dependes cache with rpmhash-based
  providename cache (based on rpm.org changes by Panu Matilainen).

* Sat Jul 03 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.38
- shebang.req (ShebangReq): fixed the check for absolute pathname
  introduced in previous release (closes: #23716).

* Thu Jul 01 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.37
- shebang.req (ShebangReq): Ensure that interpreter is specified
  as an absolute pathname (closes: #20096).
- platform.in: Enabled %%check in buildreq mode (closes: #23030).
- pkgconfig.req (PkgconfigReqProv): Relaxed version check.

* Wed Apr 21 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.36
- rpmdb.c: Make rpmdb index list hard-wired, remove unused require-
  and provideversion indexes (Panu Matilainen).

* Wed Apr 14 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.35
- brp-verify_elf: Disabled lint check on ARM.

* Thu Apr 08 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.34
- find-package: Introduced FINDPACKAGE-COMMANDS output.

* Sat Mar 27 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.33
- brp-strip: Speed up by optimizing file(1) invocations.

* Fri Mar 26 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.32
- build/files.c (checkHdrIntersect): Avoid quadratic behaviour.

* Tue Mar 16 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.31
- rpmpopt.in: Do remove extra newline before %%{CHANGELOGTEXT} (4.0.4-alt93).

* Tue Mar 16 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.30
- order.c: Factored from depends.c.
- order.c: Added missing error message.

* Fri Mar 12 2010 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.29
- al.c: Introduced "fasthash" to improve bsearch performance.

* Wed Jan 13 2010 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.28
- lib.req: Recognize STB_GNU_UNIQUE symbols and add rtld(GNU_UNIQUE)
  requirement for objects that contain such symbols.
- platform.in: Added %%_aclocaldir and %%_locksubsysdir macros
  (closes: #22710).

* Mon Dec 21 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.27
- brp-bytecompile_python: Hardlink identical .pyc and .pyo files.

* Sun Dec 20 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.26
- rpmrc.c (is_pentiumN): Added models with nonzero extended model
  (reported by Alexander Sharapov).
- brp-verify_elf: Fixed typo introduced by previous release.
- brp-verify_elf: Added "default" mode.
- verify-elf: Omit duplicate lines from eu-findtextrel's output.

* Sun Dec 20 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.25
- verify-elf: Implemented new "lint" method using "eu-elflint --gnu-ld";
  for now, enabled lint=relaxed mode (just warnings) by default.
- verify-elf: Improved "textrel" diagnostics by using eu-findtextrel.
- verify-elf: Fixed typos in "rpath" method.

* Tue Dec 01 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.24
- Recoded README.ALT-ru_RU.KOI8-R -> README.ALT-ru_RU.UTF-8.
- Rebuilt with python-2.6.x.

* Sun Oct 04 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.23
- depends.c: avoid expression dependent on evaluation order
- depends.c: implemented automatic realloc
- al.c: factored from depends.c
- al.c: reimplemented alProvIndex and alDirIndex/alFileIndex routines

* Thu Oct 01 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.22
- Removed support for availablePackages/suggestedPackages.
- Removed rebuilddb db_filter_dups code (Panu Matilainen).

* Tue Sep 29 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.21
- rpmio, rpmbuild: Added support for .xz/.lzma compressed sources and patches.
- Removed old scripts in /usr/lib/rpm.

* Sat Sep 26 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.20
- rpmio: Tweak lzma preset options for better compression.

* Thu Sep 24 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.19
- rpmio: Updated lzma compression routines for xz-5.0 API.
- Packaged /usr/bin/rpm2cpio.static.

* Wed Sep 09 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.18
- Backported %%check from rpm-4.2.
- Implemented automated %%check control using
  --enable/--disable/--with/--without check/test controls.
- Bumped librpmbuild soname to reflect ABI change intoduced
  along with %%check support.

* Tue Jul 14 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.17
- rpmio/macro.c (doShellEscape): Fixed potential buffer underflow (closes: #11921).

* Wed Jul 01 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.16
- find-package: Removed contents_index_all search, enabled file-level dependencies.

* Fri Jun 26 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.15
- pam.req.in:
  + Handle lines with optional modules.
  + Handle lines with leading whitespaces.
  + Handle lines with conditional controls.
- brp-cleanup.in wrt PAM config files:
  + Changed pam_stack replacement from "include" to "substack".
  + Changed spacing.

* Sun Jun 21 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.14
- shell.req.files: Adjusted /bin/ash script detection.

* Wed Jun 17 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.13
- Removed _noDirTokens support (producing legacy filelist format).
- Disabled rpmlib(PayloadFilesHavePrefix), rpmlib(CompressedFileNames),
  and rpmlib(VersionedDependencies) dependencies.
- Disabled versioning for rpmlib(PayloadIsBzip2) and rpmlib(PayloadIsLzma)
  dependencies.

* Sun Jun 14 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.12
- Remove artificial limit in dependency loop elimination attempts (Panu Matilainen).

* Sat Jun 13 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.11
- rpmdb: Minor fingerprint cache improvement.

* Wed May 20 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.10
- brp-compress: Avoid non-standard info directories (closes: #19993).
- rpm-build: Implemented info files verification.

* Mon May 11 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.9
- Re-enabled LZMA compression for rpm itself.

* Mon May 11 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.8
- package.c (readPackageHeaders): Use posix_fadvise(2) to disable readahead.
  When scanning a large number of packages (with e.g. rpmquery), readahead
  might cause negative effects on the buffer cache.

* Thu Apr 23 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.7
- platform.in (%%configure): Do not run libtoolize, to avoid
  "libtool version mismatch" disaster.
- python: Fixed build with libtool.

* Thu Apr 23 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.6
- rpmdb: Rebuilt with libdb4.7.

* Thu Apr 23 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.5
- rpmdb: Removed db1 support.
- db3.c (db3close): Backported fix for double close (RH#138589).

* Sat Apr 18 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt98.4
- rpm.8: Fixed typo (closes: #19356).
- platform.in: Added macros: %%_logrotatedir, %%_runtimedir (closes: #13639).
- Dropped deprecated RPMTAG_RHNPLATFORM support.
- Dropped unused RPMTAG_PLATFORM support.
- rpmVersionCompare(): Added handling of omitted tags.
- rpmevrcmp: Changed to use rpmVersionCompare() instead of rpmEVRcmp().
- 0common-files.req.list: Added /etc/X11/wms-methods.d (Igor Vlasenko).

* Thu Mar 26 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.3
- Improved fingerprint cache performance (credits: Florian Festi).

* Tue Mar 10 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.2
- Removed prehistoric multilib support.

* Mon Mar 09 2009 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt98.1
- alt97.M50 release series is for branch 5.0, alt98 is for Sisyphus.
- Updated %%config algorithm to avoid unnecessary *.rpmnew, *.rpmsave,
  and *.rpmorig files (credits: Panu Matilainen, Tomas Mraz).
  + If new package keeps the same config file, updating the file on disk
    is skipped (rhbz#194246).  This provides limited support for replacing
    config files with custom symbolic links.
  + If pre-existing file is the same as the one being installed for
    the first time, backup action is suppress (rhbz#128622).
  + Backup action is also disabled but for regular files and symlinks.

* Wed Feb 25 2009 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.16
- platform.in: Imported %%makeinstall_std macro from rpm-build-perl.
- 0common-files.req.list: Removed /etc/tex-fonts.d directory.

* Tue Dec 16 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96.15
- Added buildreq ignore rule for /usr/lib/rpm/macros.d/*

* Fri Dec 12 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.14
- psm.c (psmTriggerPosttrans):
  Handle null "transaction file info" pointer properly (closes: #18079).

* Sun Nov 23 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.13
- Introduced /usr/lib/rpm/macros.d/ (closes: #17948).
- GROUPS: Added "Graphical desktop/Sugar" (closes: #17950).
- platform.in:
  + Marked update menu, window managers registration, scrollkeeper
    database synchronization and ldconfig macros as obsolete.
  + Removed obsolete %%php_version and %%php_release macros (Alexey Gladkov).
  + Added %_rpmmacrosdir macro.

* Thu Nov 13 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.12
- verify-elf: Removed no longer needed workaround for PIE executables on ARM.
- 0common-files.req.list: Added /etc/sysconfig/limits.d (service).
- librpm, librpmbuild: Removed %%post/%%postun scripts.
- librpm: Fixed crash bug in saveTriggerFiles().

* Wed Nov 12 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96.11
- Implemented post-transaction filetriggers, loosely based on filetriggers.patch
  from Mandriva Linux (see %_rpmlibdir/posttrans-filetriggers for details)
- Implemented %_rpmlibdir/0ldconfig.filetrigger, so that packages with
  shared libraries need not to invoke ldconfig(1) in they %%post-scriptlets
- rpmlib.req: Automatically generate rpmlib(PosttransFiletriggers) dependency
  for packages which provide %_rpmlibdir/*.filetrigger programs

* Mon Nov 10 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96.10
- Improved install/upgrade package reordering (in tsort algorithm,
  changed "presentation order" to "chainsaw order")

* Tue Oct 28 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.9
- rpmevrcmp, rpmvercmp: Imported from rpm-utils (closes: #13627).
- GROUPS: Added "Development/Tools" (ALT#17550).

* Mon Oct 20 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.8
- Fixed build with new toolchain.
- brp-cleanup: Remove .gitignore files as well.
- fixup-libraries: Enhance recognition of ELF executables.

* Mon Oct 06 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96.7
- build/reqprov.c: when folding duplicate dependencies, Requires(pre)
  or Requires(post) should not opimitze out bare Requires
- build/files.c: execute find-requires before find-scriptlet-requires
- 0common-files.req.list: added /etc/rc.d/init.d (service)

* Thu Oct 02 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.6
- build/parsePrep.c (doUntar): Remove "-L" option from "unzip" invocation
  (Igor Vlasenko; closes: ALT#17407).

* Tue Sep 16 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96.5
- build/spec.c: don't allow multiple definition of PatchX or SourceX
  (Jindrich Novy, rhbz#458261, rhbz#458260)

* Sun Sep 14 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96.4
- shebang.req: check for trailing <CR> in interpreter path or name;
  also, validate argc, since neither execve(2) nor env(1) split arguments

* Fri Aug 29 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt96.3
- find-lang: Updated usage (Slava Semushin; closes: ALT#15492).
- GROUPS: Added "Development/Erlang" (ALT#16691).

* Fri Aug 29 2008 Kirill A. Shutemov <kas@altlinux.ru> 4.0.4-alt96.2
- add support of armv5tel and fix armv5tejl

* Wed Aug 20 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96.1
- find-package: updated check for file path components being alternatives

* Sun Jul 20 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt96
- lib.req: use CanonPath() to deal with RPATH like $ORIGIN/../lib
- fixup-{libtool,pkgconfig}: quote substitution text (Dmitry V. Levin, #11437)
- pdeath_execute.c: remove X_OK check, use execvp(3)
- rpm: in %post-script, remove /var/cache/apt/*.bin

* Tue Jun 24 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt95
- build/files.c: re-fixed RPMTAG_SIZE calculation (cf. #2634)
- files.req: implemented modular /usr/lib/rpm/*-files.req.list,
  for dependencies on directories

* Sun Jun 15 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt94
- %_rpmlibdir/macros: enabled LZMA payload compression (w2.lzdio) by default
- parsePreamble.c, parseSpec.c: allow "BuildArch: noarch" subpackages
- parseSpec.c: fixed duplicating 'rpmbuild -bE' output (Kirill Shutemov, #5662)
- psm.c: fixed chown attempts for src.rpm introduced in alt93
- psm.c: also, when installing src.rpm, drop suid/sgid bits
- rpmrc.c: recognize new Intel CPUs (Dmitry V. Levin)
- rpmrc.c: classify SSE2-capable Intel CPUs as "pentium4"
- find-package: corrected host-system lookup for commands

* Wed Jun 04 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt93
- rpmio.c: implemented rsyncable gzdio compression
- rpmio.c: applied SUSE patches for LZMA payload compression
  and decompression (LZMA compression disabled by default)
- transaction.c: updated conflict algorithm for overlapping paths:
  exact st_mode and uid/gid match is required; md5 check is skipped
  for %%ghost files
- build/files.c, build/parseReqs.c: allow versioned path dependencies
- build/files.c: added /usr/share/gtk-doc/html to hardcoded docDirs list
- build/files.c (isDoc): fixed docDirs match algorithm (Panu Matilainen)
- scripts, macros: use `chmod -c' where appropriate (verbosity change)
- rpmpopt (--setugids): use `chown -c -h' (no dereference)
- rpmpopt (--changelog): remove extra newline after %%{CHANGELOGNAME} (jbj)
- brp-cleanup: remove .cvsignore files (dottedmag)
- doc/manual: spelling corrections (jbj)

* Tue Apr 08 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt92
- shebang.req: Do not read the whole script (Alexey Tourbin).
- rpmReadPackageManifest: Fixed comments handling.

* Mon Mar 31 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt91
- build/files.c: check if the same files are packaged into a few subpackages

* Sun Mar 30 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt90
- reverted the rest of dependency optimization
- stripped redundant librpm-devel dependencies
- build/files.c: fixed a few possibilities of invalid cpio entries, including
  `dir/.*' construct in %%files section (which globs `dir/.' and `dir/..',
  and thus should not be used in specfiles)
- lib/psm.c: in syslog messages, discriminate between no epoch and zero epoch

* Mon Mar 03 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt89
- platform.in: Changed several macro definitions to avoid extra
  autodependencies when used in shell scripts.

* Mon Feb 25 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt88
- verify-elf: According to information from Kirill Shutemov,
  PIE executables on ARM always contain TEXTREL, so do not check them.
- find-package: Removed bulk dependencies optimization
  introduced in previous release.
- scripts: Replaced redundant paths to basic programs
  with program names.

* Sun Feb 24 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt87
- implemented automatic dependencies for %%pre, %%preun, %%post,
  and %%postun scriptlets (#7409)
- find-package: when possible, keep file-level dependencies as is,
  without mapping them to package names
- find-package: relax file-level dependencies on unpackaged directories
- find-package: optimize out bulk dependencies on sh, cat, rm, mv etc.
- build/parseScript.c: optimize out RPMSENSE_INTERP dependencies on /bin/sh
- lib.req: enabled ELF_INTERP dependencies except for standard /lib/ld-linux.so.2
- functions (ValidateBuildRoot): require RPM_BUILD_ROOT path be canonical
  (you may need to adjust %%_tmppath in ~/.rpmmacros)

* Fri Feb 22 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt86
- pam.req: Added substack support.
- Renamed rpm-python subpackage to python-module-rpm.

* Fri Feb 22 2008 Alex V. Myltsev <avm@altlinux.ru> 4.0.4-alt85
- rpm-python: fix segfaults with Python 2.5.

* Sat Jan 26 2008 Grigory Batalov <bga@altlinux.ru> 4.0.4-alt84.1
- Rebuilt with python-2.5.

* Fri Jan 18 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt84
- lib.req:
  + Fixed awk script for ldd output (at@).
  + Fixed file-level dependencies output (at@).
- relative: Fixed potential NULL dereference introduced by -alt81 (#14067).

* Tue Jan 15 2008 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt83
- platform.in:
  + Added %%warning macro.
  + Added %%autoreconf macro to replace %%__autoreconf;
  + Added deprecation warning to %%__autoreconf macro.

* Mon Jan 14 2008 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt82
- reqprov.c (addReqProv): implemented optimization of "self-requires"
  dependencies on packaged files
- find-package, shell.req, pkgconfiglib.req, symlinks.req: do not
  completely ignore dependencies on files which are under RPM_BUILD_ROOT;
  that is, emit "file-level" dependencies which will be optimized out by
  addReqProv() within a single subpackage, but will protect from unpackaged
  files between subpackages; works best with apt-utils >= 0.5.15lorg2-alt17
- lib.req: try to emit file-level dependencies instead of "soname-level"
  dependencies on private libraries (see git log for details); this can
  largely reduce the need for %%add_findprov_lib_path

* Wed Nov 21 2007 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt81
- symlinks.req: do only one single step of symlink resolution;
  find-package (FindByPath): check each path component for alternatives;
  this fixes the case of e.g. dependency on /usr/share/libtool/config.sub,
  where '/usr/share/libtool -> libtool-1.5/' is an alternative;
  we now stop at alternative directory and simply yield the dependency
  on /usr/share/libtool, instead of libtool_1.5 (see also #13374)
- moved /usr/lib/rpm/functions and /usr/lib/rpm/find-package from rpm-build
  to rpm, to relax e.g. rpm-build-mono dependencies
- relative.c: various fixes by Alex V. Myltsev and Dmitry V. Levin

* Sat Nov 10 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt80
- Merged ARM support changes made by Kirill Shutemov.
- Minor scripts/* cleanup.

* Mon Oct 29 2007 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt79
- shell.req: implemented strong self-requires elimination within
  a package; if ANY shell script defines function "foo", dependencies
  on "foo" are discarded in all shell scripts throughout the package;
  a warning is issued if e.g. /usr/bin/foo executable is available
- pkgconfiglib.req: new dependency generator for "Libs" field in *.pc
  files; maps e.g. "-lfoo" -> /usr/lib/libfoo.so -> libfoo-devel

* Sun Sep 30 2007 Alexey Tourbin <at@altlinux.ru> 4.0.4-alt78
- implemented modular subsystem for find-requires and find-provides;
  there's no separate doc yet, except for my protva2007-ru-short.pdf;
  see also git changelog, especially commit 9717c128
- improved /usr/lib/rpm/find-package algorithms
- shell.req: non-executable scripts are now processed as well as executable
  ones; also, more shebang variants are recognized, e.g. "#!/usr/bin/env bash"
- symlinks.req: new dependency generator for external symbolic links
- pkgconfig.req: pkg-config(1) errors no longer silently ignored
- rpm-build: decoupled rpm-build-tcl from the base build environment

* Tue Aug 28 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt77.M40.1
- rpmdb/header.c (guess_category_value):
  Allowed overriding locale using $RPM_LANGUAGE_I18NSTRING for header FindI18NString.
- build.c (buildForTarget):
  Changed to pass --wildcards to tar on build from tarball (RH#206841).
- GROUPS: Added "System/Legacy libraries" (#12629).
- scripts/find-package.in (FindPackage):
  Speedup index processing order by checking binary index prior to complete index.

* Fri May 18 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt77
- rpmio/macro.c (doFoo):
  Fixed potential buffer overflow in %%homedir macro processing.
- lib/formats.c (i18nTag):
  Fixed potential null dereference on header without RPMTAG_NAME (RH#239557).
- lib/package.c (readPackageHeaders):
  Removed insecure legacy providePackageNVR() call.
- lib/query.c (showQueryPackage):
  Fixed potential null dereference in QUERY_FOR_DEFAULT mode.

* Tue Apr 10 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt76
- rpmRunTransactions:
  Ignore unavailable mount points instead of bailing out.

* Wed Mar 28 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt75
- Changed /mnt/* mountpoints handling to behave the same way
  as other mountpoints.
- Fixed support of filesystems with f_bsize==0.
- files.req.list: Added /etc/hooks directory.

* Sat Mar 17 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt74
- find-provides, find-requires:
  + Added support for files of type "Mono/.Net assembly" to repair
    Mono support when new file(1) is installed (#11088, ildar@).

* Thu Feb 22 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt73
- Updated gendiff utility (Alexey Tourbin).
- Added hooks for Java autoreq facility (Damir Shayhutdinov).
- build/expression.c (doPrimary): Read closing parenthesis (Michael Schroeder).
- files.req.list: Added /etc/udev/rules.d directory.
- Changed default nice change value from 10 to 8.
- Made nice change value configurable via %%nice_change macro.

* Thu Jan 11 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt72
- rpmrc.in: Use -mtune=i686 instead of -mtune=generic for i[3456]86
  (same as generic in gcc4.1, better backwards compatibility).

* Thu Jan 11 2007 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt71
- platform.in: Changed %%optflags_kernel to %%nil.
- rpmrc.in: Changed %%optflags_default to use -mtune=generic
  instead of -mtune=pentium4 for i[3456]86 (Alexey Tourbin).

* Thu Nov 30 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt70
- platform.in: Add %%_target_libdir macro.

* Sun Nov 19 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt69
- GROUPS: New group: Graphical desktop/Rox (#10268).
- Makefile.am: Link rpm.static with -pthread.
- lib/query.c: Flush query format buffer before listing files (CVE-2006-5466).
- build/parsePrep.c:
  + Change %%patch to be more verbose by default, introduce -s option
    to make %%patch as silent as before this change (#10261).
  + Change %%setup to enable -q option by default, introduce -v option
    to make %%setup as verbose as before this change.

* Wed Oct 04 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt68
- rpmio/rpmrpc.c (Glob): Override gl_stat to allow broken symlinks.
- Implemented mono reqprov hooks and enabled them by default,
  based on patch from Ildar Mulyukov (#9426).

* Thu Sep 21 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt67
- autodeps/linux.req.in:FindLibReqs():
  If object contains .gnu.hash section but does not
  contain .hash section, add rtld(GNU_HASH) requirement.
- GROUPS: Removed trailing whitespaces (#9963).
- Rename athlonxp platform to athlon_xp (#9991).
- scripts/brp-compress.in:
  Recognize "false|no|none|off" as well as "skip" (#9854).
- scripts/brp-strip.in:
  Recognize "skip" as well as "false|no|none|off" (#9854).
- rpmdb: Honor rpmdbInit() return code (#9406).
- rpmQueryVerify(): when rpmReadPackageManifest() is disabled,
  treat RPMRC_BADMAGIC return code from rpmReadPackageHeader()
  like other read errors (#9433).
- showMatches(): Backported --querybynumber looping fix (#9773).

* Sun May 14 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt66
- Fixed build with gcc-4.1.0.

* Tue Apr 04 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt65
- build/parsePreamble.c:
  + Ignore RPMTAG_BUILDROOT value completely.
- brp-verify_elf, verify-elf:
  + Implemented VERIFY_ELF_STACK=normal (lakostis).
- platform.in:
  + Set %%_verify_elf_method to
    arch=normal,fhs=normal,rpath=normal,stack=normal,textrel=normal,unresolved=normal

* Mon Mar 20 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt64
- rpmrc.c:
  + Backported defaultMachine changes from rpm-4_4 branch.
- rpmrc.in:
  + Added pentium2, pentium3, athlonxp.
  + Replaced -mcpu=i686 with -mtune=pentium4.
  + Added -mtune=athlon-xp for k6-compatibles.
- installplatform, macros.in:
  + Updated for new arches.
- Updated libdb4 build requirements.

* Thu Mar 09 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt63
- verify-elf:
  + Extended VERIFY_ELF_UNRESOLVED=normal to behave like strict
    for executables too.

* Tue Mar 07 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt62
- platform.in:
  + %%configure: Export lt_cv_deplibs_check_method=pass_all.
  + %%_verify_elf_method: Changed unresolved from relaxed to normal.
- brp-verify_elf, verify-elf:
  + Implemented VERIFY_ELF_UNRESOLVED=normal (like strict
    for standard paths and relaxed for others).

* Thu Feb 23 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt61
- find-provides:
  + Applied "new style provides" rules for sonameless libraries too.
- lib/rpmrc.c:
  + Extended %%_preScriptEnvironment to export RPM_TARGET_ARCH.
- brp-verify-elf, verify-elf:
  + Implement "arch" option.
- platform.in:
  + Extended %%___build_pre to export RPM_TARGET_ARCH.
  + Set %%_verify_elf_method to
    arch=normal,fhs=relaxed,rpath=normal,textrel=normal,unresolved=relaxed

* Tue Feb 21 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt60
- verify-elf:
  + Fixed typo in VERIFY_ELF_UNRESOLVED support.
  + Prohibit rpaths starting/anding with ":" or containing "::".
- platform.in:
  + Updated %%_x11*dir macros (#8825).
  + Added %%_niconsdir, updated %%_miconsdir and %%_liconsdir (#9067).

* Wed Feb 08 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt59
- find-requires:
  + Do not strip arch-dependent path components when processing
    dependencies on libraries from non-standard locations.
- fixup-{binconfig,libtool,pkgconfig}:
  + Redone %%_libdir processing in more generic way.

* Thu Feb 02 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt58
- Implemented pkgconfig reqprov support and enabled it by default.

* Mon Jan 16 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt57
- find-requires:
  + Fix ld-linux* dependencies handling.
- verify-elf:
  + Make fhs check really work.
  + Redirect output of unresolved check to stderr.
  + Prefix all messages with WARNING or ERROR depending on check mode.

* Sat Jan 14 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt56
- ldd: New helper.
- find-provides, find-requires:
  + Limit executables and libraries to ELF objects.
- find-requires:
  + Use ldd helper instead of system ldd.
- brp-verify-elf, verify-elf:
  + Implement "fhs" and "unresolved" options.
- platform.in:
  + Set %%_verify_elf_method to
    fhs=relaxed,rpath=normal,textrel=normal,unresolved=relaxed
- Link librpmdb with -lpopt.
- Link rpmmodule.so with -lpython%%__python_version.

* Wed Jan 11 2006 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt55
- platform.in:
  + Add macros: %%_desktopdir, %%_pixmapsdir (#8767).
- find-provides:
  + Handle *.pth files like other python files
    (#8812, patch from Ivan Fedorov).
  + Remove old-style provides for sonames in non-default locations.
  + Change provides format of PAM modules from pam_module.so to
    PAM(pam_module.so).
- find-requires:
  + Change output format for dependencies on sonames in non-default
    locations (from basename style to pathname style).
- pam.req:
  + Change requires format of PAM modules from pam_module.so to
    PAM(pam_module.so).
- dump_ld_config, shlib.req.awk: New helpers.

* Wed Nov 30 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt54
- build/parseSpec.c: Pass undefined macros in preprocess mode
  (patch from raorn@).
- platform.in:
  + Added %%__autoreconf macro (#8307);
  + Added --disable-dependency-tracking to %%configure (#8558).
- rpminit, rpminit.1: Added public domain statements (#8433).
- python/Makefile.am: Fixed x86_64 support.
- Enabled build of python subpackage for x86_64.

* Wed Oct 19 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt53
- librpmdb: reverted incompatible part of rpmTagTable backport.
  Reported by Alexey Tourbin.

* Sat Oct 15 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt52
- Relocated some code from librpm to librpmdb, do resolve
  undefined references between libraries.
- domd5(): Backported prelink support.
- Changed build to link librpmdb with libelf by default.
- Set umask 022 for install scripts and triggers execution.
- Backported epoch handling fix to package upgrade algorithm.
- Backported my own changes to the package upgrade algorithm:
  + Remove old files on "-U --force" even if package NEVRs match.
  + When comparing package versions on -U or -F, take
    build dates into account.

* Thu Oct 13 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt51
- rpmio/macro.c, build/parseSpec:
  + When %%_allow_undefined_macros is set to true value,
    pass undefined macros but issue warning messages.
  + When processing section where comments should be skipped,
    pass undefined macros within comments but issue warning messages.
  + When processing %%prep, %%build, %%install and %%clean sections,
    pass undefined macros and issue warning messages.
  + When processing undefined macros, issue warnings instead of errors
    for short macros which cannot be defined.

* Mon Oct 10 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt50
- parseSpec.c:
  + Added %%docdir to tags_files_list.
  + Backported nested conditionals handling fix.
  + Backported multiline macro support.
- GROUPS: New group added: Networking/FTN (closes #7718).
- rpmbuild.8: Added --nosource/--nopatch descriptions (closes #8015).
- installplatform, platform.in, rpmrc.in:
  + Maintain noarch as self-contained architecture (mouse@).

* Thu Sep 29 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt49
- Changed expandMacro() and related callers to print error message
  and set error status for undefined macros (closes #8089).
  Introduced %%_allow_undefined_macros to pass undefined macros.
- Fixed rpmExpand* usage everywhere.
- platform.in: Fixed %% quotation.
- strip_files: Removed StripNote() code.

* Mon Sep 05 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt48
- brp-compress: handle RPM_COMPRESS_SKIPLIST environment variable.
- parseScript: do not generate RPMSENSE_INTERP dependencies
  when autoReq is disabled.
- Corrected license tags (#6710).

* Fri Jul 01 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt47
- shell.req:
  redirected test run of "bash --rpm-requires" to /dev/null.

* Wed Jun 29 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt46
- lib/header.c:
  parseFormat(): allocate necessary memory for arrays
  (closes #6086).
- GROUPS:
  new groups: Development/Documentation, Documentation
  (closes #7182).
- shell.req:
  use "bash" for Bourne-Again shell scripts, and "sh" for others
  (closes #7242).

* Thu Jun 16 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt45
- Added x86_64 support (Mouse, closes #4903).
- Build this package without optimizations based on strict aliasing rules.

* Wed Jun 15 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt44
- librpmdb: Fixed locking issue (#990).
- rpm-build: Removed net-tools from dependencies.
- platform.in: new macro: %%_rpmlibdir.

* Tue May 10 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt43
- Rebuilt with glibc-2.3.5 and python-2.4.

* Thu Feb 10 2005 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt42
- Backported db-4.3 support.
- GROUPS: new group: System/X11.
- platform.in:
  + updated %%configure.
  + new macros: %%_x11x11dir, %%_pkgconfigdir.
  + export RPM_LIB and RPM_LIBDIR variables.
- pam.req: initial mulitlib support.
- brp-cleanup: fixed "find -maxdepth" warning.
- find-lang: made --custom-* options work both as script and script-file.

* Sun Oct 31 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt41
- brp-bytecompile_python: check that $RPM_PYTHON is executable (#4756).
- find-lang: changed --with-man mode (#5164).
- brp-fixup: fixed typo (#5273).
- platform.in: updated python support (Andrey Orlov, #5291).
- Added pentium4 arch support (Sir Raorn, #5259).
- Added tcl findreqprov support (Sergey Bolshakov, #5364).

* Tue Jun 29 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt40
- find-lang:
  + more tweaks (#4540).
  + more options (#3244).

* Sun Jun 27 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt39
- rpmio/macro.c(grabArgs):
  + fixed to avoid newline eat up (#366).
- lib/header.c:
  + changed headerFindI18NString() and others to follow
    the gettext(3) rules (#1379).
- build.c(buildForTarget):
  + implemented %%_buildrequires_build support.
- find-lang:
  + corrected regexps (#4228).
- platform:
  + %%set_*_version: update %%_buildrequires_build (#3335);
  + run scrollkeeper-update quietly (#4485);
  + fixed typo in %%add_python_lib_path().
- find-provides:
  + parse unrecognized __init__.py files as python files,
    patch from Andrey Orlov.

* Mon May 17 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt38
- Disallow root to install source packages by default.
- find-lang: handle symlinks in --with-gnome mode.
- find-requires:
  + updated hooks for python support, from Andrey Orlov.
- brp-bytecompile_python:
  + use new bytecompiler, from Andrey Orlov.
- platform:
  + added python to default lists of find{req,prov} methods.

* Mon Apr 26 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt37
- build/parseReqs.c(parseRCPOT): better error reporting (#3883).
- fixup-libraries: recognize PIE objects.
- platform: added more python macros, from Andrey Orlov.
- find-requires, find-provides:
  + updated hooks for python support, from Andrey Orlov
    with minor tweaks.

* Mon Mar 01 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt36
- find-requires, find-provides:
  + Implemented hooks for python support, from Andrey Orlov
    with minor tweaks.

* Sun Feb 29 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt35
- Backported db-4.1 support (#3464).
- Implemented db-4.2 support.
- rpmdb: enhanced rebuilding database messages.

* Fri Feb 27 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt34
- find_lang: implemented support for symlinks in /usr/share/locale/.
- platform: added force_* macros suggested by Alexey Morozov.
- headerFindI18NString: do not translate empty strings.
- expandMacro: handle single %% properly.
- Fixed build with fresh autotools.

* Tue Feb 03 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt33
- lib/psm.c(runScript):
  + executed scripts expect default SIGPIPE handler,
    so reset it (fixes #2573).
- find-provides:
  + for symlinks to shared libraries, ignore symlinks to shorter
    locations (workaround for libdb-4.0.so provides problem).
- macros:
  + fixed %%__cxx macro definition (reported by aris@),
    was broken since 4.0.4-alt29.

* Thu Jan 29 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt32
- find-provides: changed output format of extra provides
  for sonames found in non-default locations
  (introduced in 4.0.4-alt30).
- build/reqprov.c(addReqProv):
  + enhanced duplicates elimination algorithm,
    it now covers all known optimization cases;
  + implemented %%_deps_optimization support.
- Updated README.ALT-ru_RU.KOI8-R.

* Tue Jan 27 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt31
- build/parseReqs.c(parseRCPOT):
  + tokens must not contain '%%<=>' symbols since it is common
    packaging error.
- build/reqprov.c(compare_deps):
  + fixes in duplicates detection algorithm introduced in
    previous release.
- build/reqprov.c(addReqProv):
  + enhanced duplicates elimination algorithm;
    it should cover most optimization cases.

* Sun Jan 25 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt30
- Reviewed all shell helpers for unneeded pattern
  substitutions (#2743).
- find-provides: output extra provides for sonames found in
  non-default locations.
- build/parseReqs.c(parseRCPOT):
  tokens must not contain '%%' symbol since it is common
  macros manipulation error.
- build/reqprov.c(addReqProv):
  + rewritten duplicates detection algorithm;
  + implemented "provided requires" detection.
- Build python module with latest python.

* Sun Jan 04 2004 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt29
- brp-cleanup: fixed possible cleanup misses.
- brp-cleanup, platform: implemented %%_keep_libtool_files support.
- verify-elf: verify SUID/SGID ELF objects as well.
- fixup-libraries: fix SUID/SGID libraries as well.
- find-lang: implemented --with-kde option (aris@, #2666).
- find-provides: simplify check for perl files (at@ request).
- rpmd, rpmi, rpmk: do not link with librpmbuild.
- /bin/rpm: build dynamically and relocate to %_bindir;
  provide symlink for compatibility.
- /usr/bin/rpm.static: package separately.
- /usr/lib/librpmbuild-4.0.4.so: package separately.
- Relocated %_rpmlibdir/{rpmrc,macros} to librpm subpackage.
- Removed c++ from build dependencies.
- lib/depends.c(rpmRangesOverlap):
  changed algorithm so EVRs will be compared
  if at least one of compared packages has EVR information.
- lib/depends.c(rangeMatchesDepFlags,alAllSatisfiesDepend):
  when using rpmRangesOverlap for versioned requires, ensure that
  provides are also versioned.

* Mon Nov 24 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt28
- brp-verify_elf:
  "%%set_verify_elf_method relaxed" now affects textrel as well as rpath.
- verify-elf:
  print textrel information even if textrel=relaxed.
- pam.{prov,req}: better error diagnostics.
- platform: corrected %%__python_version definition (#3311).
- Fixed Makefiles to correct librpm*-4.0.4.so dependencies.
- Do not package .la files.
- brp-cleanup: remove lib*.la files from /lib, /usr/lib, and /usr/X11R6/lib.
- brp-fix-perms, fixup-libraries:
  + strip executable bit from non-executable libraries;
  + ensure that file objects in /usr/ are user-writable.
- rpmbuild --rebuild/--recompile: implemented support for new macros:
  %%_rpmbuild_clean and %%_rpmbuild_packagesource.
- Updated README.ALT-ru_RU.KOI8-R.

* Sun Nov 09 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt27
- helper shell scripts:
  + use printf instead of echo where appropriate;
  + moved common code to %_rpmlibdir/functions.
- Implemented %%_unpackaged_files_terminate_build support.
- rpm-build: do not package %_rpmlibdir/mkinstalldirs.
- Do not package build-topdir subpackage by default.
- verify_elf: implemented TEXTREL checking.
- Updated README.ALT-ru_RU.KOI8-R.

* Sat Sep 27 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt26
- gendiff: cleanup (#2558).
- build/files.c: fixed RPMTAG_SIZE calculation (#2634).
- New group: Graphical desktop/XFce (#3048).
- platform.in(%%configure):
  + invoke libtoolize when configure.ac is present (#3049).
- pam.prov:
  + validate $PAM_NAME_SUFFIX.
- pam.req:
  + validate $PAM_SO_SUFFIX and $PAM_NAME_SUFFIX;
  + induce "buildreq -bi" to generate dependence on
    libpam-devel package (#3050).
- Updated README.ALT-ru_RU.KOI8-R.

* Mon Sep 22 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt25
- find-package:
  + when dependence name starts with `/',
    look into pkg contents binary index as well;
  + fixed package database checks.
- perl.{req,prov}: relocated to separate subpackage.
- tcl.req: fixed perl syntax (at).

* Fri Sep 12 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt24
- rpm-build: do not package %_rpmlibdir/config.* files (#2732).
- build/pack.c: create %%_srcrpmdir (#2353).
- rpmrc.in:
  + added armv5 arch support (#2801, Sergey Bolshakov).
- configure.in:
  + fixed build without python (#2802, Sergey Bolshakov).
- perl.{req,prov}:
  + new version from perl maintainer (Alexey Tourbin).

* Sat Aug 16 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt23
- autogen.sh:
  + removed all autotools restrictions.
- platform.in:
  + fixed typo in %%_scripts_debug support.
  + %%optflags_warnings: added "--enable Werror" support.
- find-requires:
  + updated to support ELF objects with private flags.

* Mon Jul 21 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt22
- lib/depends.c:
  + fixed "Requires(post,preun)" problem.
- lib/psm.c:
  + do syslog only when geteuid() == 0.
- build/poptBT.c, build/rpmbuild.h, build.c, rpmqv.c:
  + implemented "rpmbuild -bM" (raorn).
- build/parsePreamble.c:
  + disabled readIcon() code (fixes #0002637).
- rpmpopt.in:
  + ignore build dependencies in "rpm* -C" (at);
  + added alias for "rpm -bM".
- librpm: stripped off executable bits from libraries.

* Fri Jun 20 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt21
- platform.in:
  + always define RPM_BUILD_ROOT;
  + define PAM_SO_SUFFIX and PAM_NAME_SUFFIX;
  + define RPM_SCRIPTS_DEBUG if %%_scripts_debug is set;
  + removed "-fexpensive-optimizations" from %%optflags_optimization
    since it's included in -O2 and -Os.
- find-provides:
  + enable shell trace mode if $RPM_SCRIPTS_DEBUG is set;
  + fixed "readlink -fv" bug introduced in 4.0.4-alt20;
  + do not ignore symlinks when parsing PAM scripts.
- find-requires:
  + enable shell trace mode if $RPM_SCRIPTS_DEBUG is set.
- find-package:
  + updated pkg contents index code.
- pam.prov:
  + honor $PAM_NAME_SUFFIX.
- pam.req:
  + honor $PAM_SO_SUFFIX and $PAM_NAME_SUFFIX.
- build/files.c:
  + honor generateDepends() return code.
- rpminit:
  + do not be verbose by default;
  + parse -v/--verbose option.

* Mon May 26 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt20
- find-provides:
  + ignore symlinks when looking for non-library provides;
  + ignore symlinks for libraries without soname;
  + for libraries with soname, ignore all but files named as soname.
- pam.req: implemented include control directive support.
- brp-cleanup: PAM configuration policy enforcement.
- Updated README.ALT-ru_RU.KOI8-R.
  
* Fri May 09 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt19
- Reduced amount of rpm subpackage dependencies.
- Moved update-alternatives to separate package.
- convertrpmrc.sh: relocated to build subpackage.
- find-requires: more filename-based autodependencies.
- find-provides: limit path where to search library provides.
- platform.in: added macros for find-provides library
  search path manipulations.
- perl.{req,prov}: new version from perl maintainer.
- brp-strip: removed perms-based lookup optimization.

* Tue May 06 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt18
- rpmio: fixed gzclose error handling.

* Thu May 01 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt17
- rpm2cpio: return proper exit code.
- Fixed perl provides autodetection (broken in -alt16).
- platform.in:
  + %%get_dep(): make valid string even for missing packages;
  + changed macros: %%post_service, %%preun_service
    (due to new info-install package).
- New group: Sciences/Medicine.
- Do not package cron and logrotate scripts.
- Updated package dependencies.

* Thu Apr 24 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt16
- Fixed segfault on "rpmquery --qf '%%{FILENAMES}' basesystem" command.
- Implemented shell functions requires/provides autodetection
  and enabled it by default.
- New groups (#0002429):
  + Development/Functional
  + Development/Haskell
  + Development/Lisp
  + Development/ML
  + Development/Scheme
- Do not build API docs by default.

* Tue Apr 22 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt15
- Fixed `rpmbuild -bE' return code (#0001021).
- platform.in:
  + export MAKEFLAGS variable (#0001796).
  + changed macros: %%post_service, %%preun_service
    (due to new service package).
- update-alternatives.8: fixed atavism (#0002273).
- Updated libdb4 build requirements.
- find-package, platform.in: added pkg contents index support.

* Sat Feb 01 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt14
- rpmio/macro.c: filter out non-alphanumeric macro files (#0001925).
- perl.req: fixed typo (#0002056).
- find-lang: added support for gnome omf files.
- build/build.c: unset all known locale environment variables
  right before executing %%___build_cmd.
- ru.po: minor translation fixes.

* Mon Dec 30 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt13
- Fixed skiplists processing.
- rpminit(1): imported from Owl with ALT adaptions.

* Sun Nov 10 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt12
- lib/query.c: rpmQueryVerify[RPMQV_RPM]: parse file argument
  (do glob and other expansions) only if glob_query is enabled
  [and disabled it by default].
  This change allows widespread constructions like
  "find -print0 |xargs -r0 rpmquery -p --".
- find-requires: fixed perl script autodetection (#0001680).
- macros:
  + Removed some obsolete macros.
  + %%___build_pre: moved to platform;
  + Added warning about misspelled architecture.
  + Added %%__spec_*_custom_{pre,post} macros.
- platform:
  + %%___build_pre: moved from macros.
  + Adjusted %%_configure_target macro,
    now uses both --build and --host options.
  + Adjusted %%clean_buildroot,
    now uses "%%__chmod -Rf u+rwX".
  + Reintroduced %%_fixperms macro,
    now uses "%%__chmod -Rf u+rwX,go-w".
  + Added CCACHE_CXX support.
- rpmpopt:
  + Added with/without/enable/disable aliases to rpmq/rpmquery.
- Fixed permissions on %_rpmlibdir in -build subpackage
  (thanks to Ivan Zakharyaschev).

* Mon Nov 04 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt11
- Fixed error handling in shell scripts.
- platform: updated %%optflags_kernel for gcc-3.2.
- find-requires: added lookup for /etc/cron.*ly.
- Updates for perl-5.8.0 migration:
  + platform: added %%_perl_req_method/%%set_perl_req_method macros.
  + macros: %%___build_pre: export RPM_PERL_REQ_METHOD.
  + perl.{req,prov}: new version (Alexey Tourbin).

* Mon Oct 28 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt10
- New macros:
  %%set_{autoconf,automake,libtool}_version.

* Fri Oct 25 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt9
- find-requires: added libperl/nolibperl options.
- New group: System/Servers/ZProducts.

* Tue Oct 22 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt8
- lib/header.c: headerFindI18NString: check for LANGUAGE first.
- perl.req: s/perl >= /perl-base >= / (Alexey Tourbin)
- Commented out old %%perl_* macros.
- Migrated to gettext-0.11.5.

* Mon Oct 07 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt7
- Fixed %%doc (was broken in -alt6).

* Sat Oct 05 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt6
- Fixed skiplists processing.
- New macro: %%_customdocdir (affects DOCDIR processing).

* Fri Oct 04 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt5
- lib/psm.c: fixed chroot(2) handling (aka "rpmi --dbpath" problem).
- po/ru.po: translation fix (#0001286).
- New method now gets executed after %%install:
  brp-fixup (controlled by %%_fixup_method macro).
- New macros:
  + %%_{cleanup,compress,fixup,strip,verify_elf,findreq,findprov}_{topdir,skiplist};
  + %%set_{cleanup,compress,fixup,strip,verify_elf,findreq,findprov}_{topdir,skiplist}();
  + %%add_{cleanup,compress,fixup,strip,verify_elf,findreq,findprov}_skiplist();
  + %%__gcc_version{,_major,_minor,_patch,_base}.
- New groups:
  + Development/Objective-C;
  + Education;
  + Games/Educational.

* Mon Sep 09 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt4
- new brp method: verify_elf.
- platform:
  + set %%_verify_elf_method to "normal";
  + added %%set_verify_elf_method() macro;
  + set %%_configure_target to "--build=%%{_target_platform}".

* Thu Sep 05 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt3
- Fixed typo in %%install_info/%%uninstall_info macros (sb).
- brp-strip:
  + added --skip-files option;
  + by default, skip all files matched by '*/debug/*' pattern.

* Mon Sep 02 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt2
- Removed README.ALT, added README.ALT-ru_RU.KOI8-R
  (based on alt-packaging/rpm.spec).
- Use subst instead of perl for build.
- find-requires: added glibc-devel-static requirement autogeneration.

* Wed Aug 28 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt1
- rpmio:
  + implemented macrofiles globbing.
  + implemented MkdirP.
- build/pack.c, lib/psm.c: make use of MkdirP for build.
- rpmpopt:
  + cloned all rpmq aliases for rpmquery;
  + added --nowait-lock alias for rpm, rpmq and rpmquery;
  + added -C alias for rpmbuild.
- platform:
  + Changed default value for _strip_method to "none" when "--enable debug" is used.
- macros:
  + added %%__subst;
  + %%___build_pre: do %%__mkdir_p %%_builddir before chdir there.
- rpmrc: added %_sysconfdir/%name/macros.d/* to macrofiles search list.
- find-requires: added /etc/rpm/macros.d dependence autodetection.
- brp-cleanup, brp-compress, brp-strip, compress_files:
  + Added parameter filtering.
- rpm: provides %_sysconfdir/%name/macros.d
- rpm-build: requires %_bindir/subst.
- New group: Graphical desktop/GNUstep.
- Moved contrib subpackage under with/without logic control and disabled
  packaging by default.
- Moved /usr/src/RPM from rpm-build subpackage to rpm-build-topdir
  subpackage (for reference; it is no longer needed).

* Mon Aug 12 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt0.11
- Fixed %%basename builtin macro.
- Implemented %%homedir builtin macro.

* Sat Aug 03 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt0.10
- Updated code to snapshot 2002-06-15 of 4_0 branch.
- Migrated to: automake >= 1.6.1, autoconf >= 2.53.
- Refined database locking patch (controlled by %%_wait_for_lock).
- update-alternatives: enhanced --config option; various fixes.
- New group: Development/Ruby.

* Mon Jul 29 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt0.9
- Dropped compatibility symlink to alt-gpgkeys
  (was added in previous release).

* Mon Jul 08 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt0.8
- Moved ALT GnuPG keyring to separate package (alt-gpgkeys).
- New rpm macros:
  subst_with();
  subst_enable().
- Merged patches from Ivan Zakharyaschev:
  - Fixed a pair of swapped function dscriptions.
  - Fixed a pair of segfaults in query format parser.
  - Added a pair of new things to the query format: 
    the '>'-test, ':nothing' format variant and 
    implemented '-q --changes-since=<e:v-r>' upon them (docs added).

* Thu Jun 13 2002 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt0.7
- Updated code to snapshot 2002-05-23 of 4_0 branch.
- runScript(): export RPM_INSTALL_ARG{1,2} variables.
- convert(): added full i18n support (it costs one more memleak).
- Support setting the BuildHost tag explicitly rather than only
  from what the kernel thinks the system's hostname is (Owl).
- find-requires: include all versioned dependencies,
  not only "GLIBC|GCC|BZLIB".
- New group: Development/Debuggers.
- Backported popt "rpm -bE" alias from rpm3 (Anton Denisov).
- New rpm macros:
  + ldconfig update (mhz):
      post_ldconfig_lib
      post_ldconfig_sys
      post_ldconfig
      postun_ldconfig
  + TCL directories (sb):
      _tcllibdir
      _tcldatadir
- %%___build_pre changes:
  + unset DISPLAY and XAUTHORITY unless explicitly redefined
    by %%_build_display and %%_build_xauthority;
  + unset CCACHE_CC and CCACHE_DIR unless explicitly redefined
    by %%__ccache_cc and %%__ccache_dir (ab).
  
* Mon Apr 22 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.4-alt0.6
- Updated code to snapshot 2002-04-19 of 4_0 branch.

* Fri Apr 12 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.4-alt0.5
- Updated code to snapshot 2002-04-11 of 4_0 branch (fixes #0000815).

* Fri Apr 05 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.4-alt0.4
- Updated code to snapshot 2002-04-04 of 4_0 branch.
- Updated gpg keyring (added: 21, dropped: 2, total: 54).
- New rpm macros:
  defined()
  undefined()
  ifndef()
  with()
  without()
  if_with()
  if_without()
  enabled()
  disabled()
  if_enabled()
  if_disabled()

* Sat Mar 30 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.4-alt0.3
- Updated code to snapshot 2002-03-27 of 4_0 branch.
- New popt aliases:
  --enable
  --disable
- New rpm macros:
  ifdef()
  check_def()
  def_with()
  def_without()
  def_enable()
  def_disable()
  post_ldconfig
  postun_ldconfig
- Honor _enable_debug macro in optflags_* definitions.
- Use postun_ldconfig.
- Automated librpm and rpm-build versioned dependencies.

* Wed Mar 27 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.4-alt0.2
- Updated russian translations.
- New macros from ab:
  rpm_check_field(p:)
  php_version(n:)
  php_release(n:)

* Mon Mar 25 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.4-alt0.1
- Updated code to snapshot 2002-03-22 of 4_0 branch.
- Updated librpm dependencies:
  libpopt >= 1:1.7-alt3, zlib >= 1.1.4, bzlib >= 1:1.0.2-alt1, libdb4.
- New macros: %%get_SVR(), %%get_dep().

* Tue Jan 29 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt3
- brp-compress.in: implemented execute permissions removal from manpages.
- brp-fix-perms: do not attempt to fix symlinks
  (fixes filesystem rebuild problem).
- brp-bytecompile_python: recompile also with optimization.
- platform.in: fixed %%__python_version definition.
- find-package: s/rpm -qf/rpmquery --whatprovides/g.
- rpmlib: do also RPMTAG_PROVIDENAME lookup for
  rpmQueryVerify(RPMQV_WHATPROVIDES) items starting with "/".

* Fri Jan 11 2002 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt2
- update-alternatives: test not for file readability but for file existance;
- new macros: update_wms, clean_wms, update_scrollkeeper, clean_scrollkeeper;
- obsolete macros: make_session.

* Mon Dec 10 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt1
- Built with new libdb3 (whith fixed chroot_hack),
  updated libdb3 dependencies; so "rpm --root" option works again.
- find-requires: fixed soname version reference requires generation
  (added GCC and BZLIB).
- Fixed russian translation (locking messages).
- Updated gpg keyring.

* Thu Dec 06 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.9.1
- Updated code to 4.0.3 release.
- rpm subpackage: fixed dependencies (glibc --> glibc-core).
- Added /usr/lib/perl5/man to default docdir list.
- Added permissions enforcing for documentation created by %%doc directive.
- Exit with nonzero if %%doc directive fails.
- Added permission policy enforcement (via brp-fix-perms script).
- Built with chroot_hack enabled, updated libdb3 dependencies.
  Beware of --root option for now.

* Mon Nov 19 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.9
- Updated requires for build subpackage.
- find-requires: added more rules for files method: logrotate, vixie-cron, chrooted.

* Fri Nov 16 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.8
- Fixed macros:
  %%configure.
- Fixed %%post script for installer and BTE.
- Fixed syslog messages (#0000157).
- Ignore icons in preprocess mode (ab).

* Tue Nov 13 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.7
- Fixed macros:
  %%remove_optflags, %%add_optflags, %%__glibc_version_minor,
  %%install_info, %%uninstall_info.
- Fixed libpopt versioned prerequires.

* Mon Nov 12 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.6
- Database locking backport: fixed error checking.
- Fixed nested boolean expressions parsing.

* Fri Nov 09 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.5
- Backported database locking (use %%_wait_for_lock to control).

* Thu Nov 08 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.4
- Updated code from 4_0 branch:
  * Mon Nov  5 2001 Jeff Johnson <jbj@redhat.com>
  - fix: big-endian's with sizeof(time_t) != sizeof(int_32) mtime broken.
  - add RPHNPLATFORM and PLATFORM tags.

* Tue Nov 06 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.3
- Corrected directory attributes.
- Made "--rebuilddb -v" more verbose.

* Mon Nov 05 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.2
- Implemented automatic db3 migration.
- Updated russian translations.

* Thu Nov 01 2001 Dmitry V. Levin <ldv@alt-linux.org> 4.0.3-alt0.1
- Initial ALT prerelease (with partial ALT specific backport from rpm3)
  based on 4.0.3 rh release 1.06.
  TODO:
  - backport database locking (--nowait-lock);
  - update russian translations;
  - implement automatic db3 migration.
