# $Id$

%define rpm_name rpm
%define rpm_version 4.0.4
%define srcname %rpm_name-4_0-%rpm_version

Name: %rpm_name
Version: %rpm_version
Release: alt21.3

%define ifdef() %if %{expand:%%{?%{1}:1}%%{!?%{1}:0}}
%define get_dep() %(rpm -q --qf '%%{NAME} >= %%|SERIAL?{%%{SERIAL}:}|%%{VERSION}-%%{RELEASE}' %1 2>/dev/null || echo '%1 >= unknown')
%define def_with() %{expand:%%{!?_with_%{1}: %%{!?_without_%{1}: %%global _with_%{1} --with-%{1}}}}
%define def_without() %{expand:%%{!?_with_%{1}: %%{!?_without_%{1}: %%global _without_%{1} --without-%{1}}}}
%define if_with() %if %{expand:%%{?_with_%{1}:1}%%{!?_with_%{1}:0}}
%define if_without() %if %{expand:%%{?_without_%{1}:1}%%{!?_without_%{1}:0}}

%def_with python
%def_without apidocs
%def_without db
%def_without contrib

# XXX enable at your own risk, CDB access to rpmdb isn't cooked yet.
%define enable_cdb create cdb

Summary: The RPM package management system
Summary(ru_RU.KOI8-R): Менеджер пакетов RPM
License: GPL
Group: System/Configuration/Packaging
Url: http://www.rpm.org/

# 1. ftp://ftp.rpm.org/pub/rpm/dist/
# 2. cvs -d :pserver:anonymous@cvs.rpm.org:/cvs/devel export -r rpm-4_0 rpm
# 3. ALT Linux CVS
Source: %srcname.tar.bz2

Provides: %_sysconfdir/%name/macros.d

PreReq: lib%name = %version-%release
PreReq: alt-gpgkeys, coreutils

# XXX glibc-2.1.92 has incompatible locale changes that affect statically
# XXX linked binaries like /bin/rpm.
Requires: glibc-core

BuildPreReq: python
%{?_with_python:BuildPreReq: python-devel = %__python_version}
%{?_with_apidocs:BuildPreReq: ctags doxygen}

BuildPreReq: automake >= 1.7.1, autoconf >= 2.53, rpm >= 3.0.6-ipl24mdk, %_bindir/subst
BuildConflicts: rpm-devel

# Automatically added by buildreq on Mon May 05 2003
BuildRequires: bzlib-devel-static cpio gcc-c++ glibc-devel-static libbeecrypt-devel-static libdb4.0-devel-static libpopt-devel-static zlib-devel-static

%package -n lib%name
Summary: Shared libraries required for applications which will manipulate RPM packages
Summary(ru_RU.KOI8-R): Файлы, необходимые для разработки приложений, взаимодействующих с RPM-пакетами
Group: System/Libraries
PreReq: zlib >= 1.1.4
PreReq: bzlib >= 1:1.0.2-alt2
PreReq: libpopt >= 1:1.7-alt3
PreReq: libbeecrypt >= 2.2.0-alt1
PreReq: libdb4.0

%package -n lib%name-devel
Summary: Development files for applications which will manipulate RPM packages
Summary(ru_RU.KOI8-R): Файлы, необходимые для разработки приложений, взаимодействующих с RPM-пакетами
Group: Development/C
Provides: %name-devel = %version-%release
Obsoletes: %name-devel
Requires: lib%name = %version-%release
Requires: bzlib-devel, libbeecrypt-devel, libdb4.0-devel, libpopt-devel, zlib-devel

%package -n lib%name-devel-static
Summary: Static libraries for developing statically linked applications which will manipulate RPM packages
Summary(ru_RU.KOI8-R): Статические библиотеки, необходимые для разработки статических приложений, взаимодействующих с RPM-пакетами
Group: Development/C
Requires: lib%name-devel = %version-%release
Requires: bzlib-devel-static, libbeecrypt-devel-static, libdb4.0-devel-static, libpopt-devel-static, zlib-devel-static

%package build
Summary: Scripts and executable programs used to build packages
Summary(ru_RU.KOI8-R): Файлы, необходимые для установки SRPM-пакетов и сборки RPM-пакетов
Group: Development/Other
Obsoletes: spec-helper
PreReq: shadow-utils
PreReq: %name = %version-%release
Requires: autoconf autoconf-common automake automake-common bison coreutils cpio
Requires: gcc gettext-tools glibc-devel file kernel-headers libtool m4 make
Requires: net-tools procps psmisc sed service sh texinfo which
Requires: bzip2 >= 1:1.0.2-alt4
Requires: gzip >= 0:1.3.3-alt2
Requires: info-install >= 0:4.5-alt2
Requires: mktemp >= 1:1.3.1
Requires: patch >= 2.5
Requires: tar >= 0:1.13.22-alt1
Requires: %_bindir/subst

%package build-topdir
Summary: RPM package installation and build directory tree
Summary(ru_RU.KOI8-R): Сборочное дерево, используемое для установки SRPM-пакетов и сборки RPM-пакетов
Group: Development/Other
PreReq: %name-build = %version-%release

%package contrib
Summary: Contributed scripts and executable programs which aren't currently used
Summary(ru_RU.KOI8-R): Файлы, не используемые в настоящее время
Group: Development/Other
PreReq: %name-build = %version-%release

%description
The RPM Package Manager (RPM) is a powerful command line driven
package management system capable of installing, uninstalling,
verifying, querying, and updating software packages.  Each software
package consists of an archive of files along with information about
the package like its version, a description, etc.

%description -l ru_RU.KOI8-R
RPM - это мощный неинтерактивный менеджер пакетов, используемый для сборки,
установки, инспекции, проверки, обновления и удаления отдельных программных
пакетов.  Каждый такой пакет состоит из набора файлов и информации о пакете,
включающей название, версию, описание пакета, и т.д.

%description -n lib%name
This package contains shared libraries required to run dynamically linked
programs manipulating with RPM packages and databases.

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

%description contrib
This package contains extra scripts and executable programs which arent
currently used.

%if_with python
%package python
Version: %{rpm_version}_%__python_version
Summary: Python bindings for apps which will manipulate RPM packages
Summary(ru_RU.KOI8-R): Интерфейс для разработки Python-приложений, взаимодействующих с RPM-пакетами
Group: Development/Python
PreReq: lib%name = %rpm_version-%release
Requires: python = %__python_version

%description python
The %name-python package contains a module which permits applications
written in the Python programming language to use the interface
supplied by RPM (RPM Package Manager) libraries.

This package should be installed if you want to develop Python
programs that will manipulate RPM packages and databases.
%endif #with python

%prep
%setup -q -n %srcname

find -type d -name CVS -print0 |
	xargs -r0 %__rm -rf --
find -type f \( -name .cvsignore -o -name \*~ -o -name \*.orig \) -print0 |
	xargs -r0 %__rm -f --

%build
./autogen.sh --noconfigure
export \
	ac_cv_path_CTAGS=/usr/bin/ctags \
	ac_cv_path_UNZIPBIN=/usr/bin/unzip \
	ac_cv_path___GPG=/usr/bin/gpg \
	ac_cv_path___SSH=/usr/bin/ssh \
	#
%configure \
	%{?_with_python} %{?_without_python} \
	%{?_with_apidocs} %{?_without_apidocs} \
	%{?_with_db} %{?_without_db} \
	--program-transform-name=

# fix buggy requires if any
find scripts -type f -print0 |
	xargs -r0 %__grep -EZl '(/usr/ucb|/usr/local/bin/perl|/usr/local/lib/rpm/bash)' -- |
	xargs -r0 subst 's|/usr/ucb|%_bindir|g;s|/usr/local/bin/perl|/usr/bin/perl|g;s|/usr/local/lib/rpm/bash|/bin/sh|g' --
find -type f -print0 |
	xargs -r0 %__grep -FZl /usr/sbin/lsattr -- |
	xargs -r0 subst 's|/usr/sbin/lsattr|/usr/bin/lsattr|g' --

# fix vendor
find -type f -print0 |
	xargs -r0 %__grep -FZl '%_host_cpu-pc-%_host_os' -- |
	xargs -r0 subst 's/%_host_cpu-pc-%_host_os/%_host_cpu-alt-%_host_os/g' --
find -type f -name config.\* -print0 |
	xargs -r0 %__grep -FZl '=pc' -- |
	xargs -r0 subst 's/=pc/=alt/g' --

%make_build YACC='bison -y'
%if_with apidocs
%__rm -rf apidocs
make apidocs
%endif #with apidocs

%install
%make_install DESTDIR="$RPM_BUILD_ROOT" install
%__chmod a-x $RPM_BUILD_ROOT%_libdir/lib*.so
%__chmod a-w $RPM_BUILD_ROOT%_usrsrc/RPM{,/RPMS/*}

# Save list of packages through cron.
#%__mkdir_p $RPM_BUILD_ROOT%_sysconfdir/cron.daily
#%__install -p -m750 scripts/%name.daily $RPM_BUILD_ROOT%_sysconfdir/cron.daily/%name
#
#%__mkdir_p $RPM_BUILD_ROOT%_sysconfdir/logrotate.d
#%__install -p -m640 scripts/%name.log $RPM_BUILD_ROOT%_sysconfdir/logrotate.d/%name

%__mkdir_p $RPM_BUILD_ROOT%_sysconfdir/%name/macros.d
touch $RPM_BUILD_ROOT%_sysconfdir/%name/macros
cat << E_O_F > $RPM_BUILD_ROOT%_sysconfdir/%name/macros.db1
%%_dbapi		1
E_O_F
cat << E_O_F > $RPM_BUILD_ROOT%_sysconfdir/%name/macros.cdb
%{?enable_cdb:#%%__dbi_cdb	%enable_cdb}
E_O_F

%__mkdir_p $RPM_BUILD_ROOT%_localstatedir/%name
for dbi in \
	Basenames Conflictname Dirnames Group Installtid Name Providename \
	Provideversion Removetid Requirename Requireversion Triggername \
	Sigmd5 Sha1header Filemd5s Packages \
	__db.001 __db.002 __db.003 __db.004 __db.005 __db.006 __db.007 \
	__db.008 __db.009
do
    touch "$RPM_BUILD_ROOT%_localstatedir/%name/$dbi"
done

# Prepare documentation.
bzip2 -9 CHANGES ||:
%__mkdir_p $RPM_BUILD_ROOT%_docdir/%name-%rpm_version
%__install -p -m644 CHANGES* CREDITS README README.ALT* RPM-GPG-KEY RPM-PGP-KEY TODO \
	$RPM_BUILD_ROOT%_docdir/%name-%rpm_version/
%__cp -a doc/manual $RPM_BUILD_ROOT%_docdir/%name-%rpm_version/
%__rm -f $RPM_BUILD_ROOT%_docdir/%name-%rpm_version/manual/{Makefile*,buildroot}
%if_with apidocs
%__cp -a apidocs/man/man3 $RPM_BUILD_ROOT%_mandir/
%__cp -a apidocs/html $RPM_BUILD_ROOT%_docdir/%name-%rpm_version/apidocs/
%endif #with apidocs

# rpminit(1).
%__install -pD -m755 rpminit $RPM_BUILD_ROOT%_bindir/rpminit
%__install -pD -m644 rpminit.1 $RPM_BUILD_ROOT%_man1dir/rpminit.1

# Valid groups.
%__install -p -m644 GROUPS $RPM_BUILD_ROOT%_libdir/%name/

# buildreq ignore rules.
install -pD -m644 rpm-build.buildreq $RPM_BUILD_ROOT%_sysconfdir/buildreqs/files/ignore.d/rpm-build

chmod a+x scripts/find-lang
# Manpages have been moved to their own packages.
#./scripts/find-lang --with-man %name rpm2cpio --output %name.lang
./scripts/find-lang %name rpm2cpio --output %name.lang

pushd $RPM_BUILD_ROOT%_libdir/%name
	for f in *-alt-%_target_os; do
		n=`echo "$f" |%__sed -e 's/-alt//'`
		[ -e "$n" ] || %__ln_s "$f" "$n"
	done
popd

/bin/ls -1d $RPM_BUILD_ROOT%_libdir/%name/*-%_target_os |
	%__grep -Fv /brp- |
	%__sed -e "s|^$RPM_BUILD_ROOT|%attr(-,root,%name) |g" >>%name.lang

%pre
if [ -f %_localstatedir/%name/Packages -a -f %_localstatedir/%name/packages.rpm ]; then
    echo "
You have both
	%_localstatedir/%name/packages.rpm	db1 format installed package headers
	%_localstatedir/%name/Packages		db3 format installed package headers
Please remove (or at least rename) one of those files, and re-install.
" >&2
    exit 1
fi

%post
if [ -f %_localstatedir/%name/packages.rpm ]; then
	%__chgrp %name %_localstatedir/%name/*.rpm
	# Migrate to db3 database.
	%_libdir/%name/pdeath_execute $PPID %_libdir/%name/delayed_rebuilddb
elif [ -f %_localstatedir/%name/Packages ]; then
	%__chgrp %name %_localstatedir/%name/[A-Z]*
	# Undo db1 configuration.
	%__rm -f %_sysconfdir/%name/macros.db1
	[ -n "$DURING_INSTALL" -o -n "$BTE_INSTALL" ] ||
		%_libdir/%name/pdeath_execute $PPID %_libdir/%name/delayed_rebuilddb
else
	# Initialize db3 database.
	%__rm -f %_sysconfdir/%name/macros.db1
	%_bindir/rpmdb --initdb
fi
:

%post -n lib%name -p /sbin/post_ldconfig
%postun -n lib%name -p /sbin/postun_ldconfig

%files -n lib%name
%_libdir/*-*.so*

%files -n lib%name-devel
%_libdir/librpm.so
%_libdir/librpmdb.so
%_libdir/librpmio.so
%_libdir/librpmbuild.so
%_libdir/*.la
%_includedir/%name
%if_with apidocs
%_man3dir/*
%dir %_docdir/%name-%rpm_version
%_docdir/%name-%rpm_version/apidocs
%endif #with apidocs

%files -n lib%name-devel-static
%_libdir/*.a

%define rpmattr %attr(755,root,%name)
%define rpmdirattr %attr(2755,root,%name) %dir
%define rpmdatattr %attr(644,root,%name)
%define rpmdbattr %attr(644,root,%name) %verify(not md5 size mtime) %ghost %config(missingok,noreplace)

%files -f %name.lang
%dir %_docdir/%name-%rpm_version
%_docdir/%name-%rpm_version/[A-Z]*
%_docdir/%name-%rpm_version/manual
%rpmattr /bin/rpm

#%config(noreplace,missingok) %_sysconfdir/cron.daily/%name
#%config(noreplace,missingok) %_sysconfdir/logrotate.d/%name

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

%rpmattr %_bindir/rpm2cpio
%_bindir/rpmdb
%_bindir/rpm[eiu]
%_bindir/rpmsign
%_bindir/rpmquery
%_bindir/rpmverify
%_bindir/rpminit

%rpmdirattr %_libdir/%name
%rpmattr %_libdir/%name/delayed_rebuilddb
%rpmattr %_libdir/%name/pdeath_execute
%rpmattr %_libdir/%name/rpm[dikq]
%_libdir/%name/rpm[euv]
%rpmdatattr %_libdir/%name/macros
%rpmdatattr %_libdir/%name/rpmpopt*
%rpmdatattr %_libdir/%name/rpmrc
%rpmdatattr %_libdir/%name/GROUPS
%_libdir/rpmpopt
%_libdir/rpmrc

%_man1dir/rpminit.*
%_man8dir/rpm.*
%_man8dir/rpm2cpio.*

%files build
%config %_sysconfdir/buildreqs/files/ignore.d/*
%rpmattr %_bindir/gendiff
%_bindir/rpmbuild
%_bindir/relative
%rpmdirattr %_libdir/%name
%_libdir/%name/rpmt
%rpmattr %_libdir/%name/rpmb
%rpmattr %_libdir/%name/brp-*
%rpmattr %_libdir/%name/*_files
%rpmattr %_libdir/%name/config.guess
%rpmattr %_libdir/%name/config.site
%rpmattr %_libdir/%name/config.sub
%rpmattr %_libdir/%name/mkinstalldirs
%rpmattr %_libdir/%name/convertrpmrc.sh
%rpmattr %_libdir/%name/rpm2cpio.sh
%rpmattr %_libdir/%name/filesize
%rpmattr %_libdir/%name/relative
%rpmattr %_libdir/%name/find-lang
%rpmattr %_libdir/%name/find-package
%rpmattr %_libdir/%name/find-provides
%rpmattr %_libdir/%name/find-requires
%rpmattr %_libdir/%name/fixup-*
%rpmattr %_libdir/%name/http.req
%rpmattr %_libdir/%name/files.*
%rpmattr %_libdir/%name/pam.*
%rpmattr %_libdir/%name/perl.*
%rpmattr %_libdir/%name/shell.*
%rpmattr %_libdir/%name/sql.*
%rpmattr %_libdir/%name/tcl.*
%rpmattr %_libdir/%name/verify-elf
%rpmattr %_libdir/%name/Specfile.pm

%_mandir/man?/gendiff.*
%_man8dir/rpmbuild.*

%files build-topdir
%attr(0755,root,%name) %dir %_usrsrc/RPM
%attr(0770,root,%name) %dir %_usrsrc/RPM/BUILD
%attr(2770,root,%name) %dir %_usrsrc/RPM/SPECS
%attr(2770,root,%name) %dir %_usrsrc/RPM/SOURCES
%attr(2775,root,%name) %dir %_usrsrc/RPM/SRPMS
%attr(0755,root,%name) %dir %_usrsrc/RPM/RPMS
%attr(2775,root,%name) %dir %_usrsrc/RPM/RPMS/*

%if_with python
%files python
%_libdir/python*/site-packages/*module.so
%endif #with python

%if_with contrib
%files contrib
%rpmattr %dir %_libdir/%name
%rpmattr %_libdir/%name/cpanflute*
%rpmattr %_libdir/%name/cross-build
%rpmattr %_libdir/%name/find-prov.pl
%rpmattr %_libdir/%name/find-provides.perl
%rpmattr %_libdir/%name/find-req.pl
%rpmattr %_libdir/%name/find-requires.perl
%rpmattr %_libdir/%name/get_magic.pl
%rpmattr %_libdir/%name/getpo.sh
%rpmattr %_libdir/%name/javadeps
%rpmattr %_libdir/%name/magic.*
%rpmattr %_libdir/%name/rpmdiff*
%rpmattr %_libdir/%name/trpm
%rpmattr %_libdir/%name/u_pkg.sh
%rpmattr %_libdir/%name/vpkg-provides.sh
%rpmattr %_libdir/%name/vpkg-provides2.sh
%endif #with contrib

%changelog
* Sun Jul 20 2003 Dmitry V. Levin <ldv@altlinux.org> 4.0.4-alt21.3
- lib/depends.c: fixed "Requires(post,preun)" problem.
- lib/psm.c: do syslog only when geteuid() == 0.
- build/poptBT.c, build/rpmbuild.h, build.c, rpmqv.c:
  + implemented "rpmbuild -bM" (raorn).
- build/parsePreamble.c: disabled readIcon() code (fixes #0002637).
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
  + honor %PAM_SO_SUFFIX and $PAM_NAME_SUFFIX.
- build/files.c: honor generateDepends() return code.
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
- Fixed segfault on "rpmquery --qf '%{FILENAMES}' basesystem" command.
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
- Fixed permissions on %_libdir/%name in -build subpackage
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
  %set_{autoconf,automake,libtool}_version.

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
- New macro: %_customdocdir (affects DOCDIR processing).

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
  + added %set_verify_elf_method() macro;
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
- Moved %_srcdir/RPM from rpm-build subpackage to rpm-build-topdir
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
