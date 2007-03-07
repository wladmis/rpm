# Makefile for rpm scripts.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = \
	brp-adjust_libraries brp-alt brp-bytecompile_python \
	brp-cleanup brp-compress brp-fix-perms brp-fixup brp-strip brp-verify_elf \
	compress_files check-files convertrpmrc.sh cross-build \
	delayed_rebuilddb find-lang find-package find-prov.pl find-req.pl \
	cpanflute cpanflute2 Specfile.pm find-provides.perl find-requires.perl \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	get_magic.pl getpo.sh http.req \
	files.req files.req.files files.req.list \
	functions magic.prov magic.req \
	ldd lib.req lib.req.files lib.prov lib.prov.files shlib.req.awk \
	pam.prov pam.req perl.prov perl.req \
	pkgconfig.req pkgconfig.req.files pkgconfig.prov pkgconfig.prov.files \
	rpmdiff rpmdiff.cgi rpm.daily rpm.log rpm.xinetd rpm2cpio.sh \
	shebang.req shebang.req.files \
	shell.req shell.req.files shell.prov shell.prov.files \
	sql.prov sql.req strip_files \
	static.req static.req.files \
	tcl.req trpm u_pkg.sh verify-elf vpkg-provides.sh vpkg-provides2.sh

installprefix = $(DESTDIR)

all:

configdir = ${prefix}/lib/rpm
config_SCRIPTS = \
	brp-adjust_libraries brp-alt brp-bytecompile_python \
	brp-cleanup brp-compress brp-fix-perms brp-fixup brp-strip brp-verify_elf \
	compress_files check-files convertrpmrc.sh cross-build \
	delayed_rebuilddb find-lang find-package find-prov.pl find-req.pl \
	cpanflute cpanflute2 Specfile.pm find-provides.perl find-requires.perl \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	get_magic.pl getpo.sh http.req \
	files.req files.req.files files.req.list \
	functions magic.prov magic.req \
	ldd lib.req lib.req.files lib.prov lib.prov.files shlib.req.awk \
	pam.prov pam.req perl.prov perl.req \
	pkgconfig.req pkgconfig.req.files pkgconfig.prov pkgconfig.prov.files \
	rpmdiff rpmdiff.cgi rpm.daily rpm.log rpm.xinetd rpm2cpio.sh \
	shebang.req shebang.req.files \
	shell.req shell.req.files shell.prov shell.prov.files \
	sql.prov sql.req strip_files \
	static.req static.req.files \
	tcl.req trpm u_pkg.sh verify-elf vpkg-provides.sh vpkg-provides2.sh
