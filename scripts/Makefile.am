# Makefile for rpm scripts.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = \
	posttrans-filetriggers 0ldconfig.filetrigger \
	debuginfo.req debuginfo.req.files debuginfo.prov debuginfo.prov.files \
	functions find-package .provides.sh \
	find-debuginfo-files \
	find-scriptlet-requires \
	brp-adjust_libraries brp-alt brp-bytecompile_python \
	brp-cleanup brp-compress brp-debuginfo \
	brp-fix-perms brp-fixup brp-strip \
	brp-verify_elf brp-verify-info \
	compress_files \
	cpp.req cpp.req.files \
	find-lang \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	files.req files.req.files 0common-files.req.list \
	ldd lib.req lib.req.files lib.prov lib.prov.files shlib.req.awk \
	pam.req pam.req.files pam.prov pam.prov.files \
	pkgconfig.req pkgconfiglib.req pkgconfig.req.files \
	rpmlib.req rpmlib.req.files \
	rpm2cpio.sh \
	shebang.req shebang.req.files \
	shell.req shell.req.files shell.prov shell.prov.files \
	strip_files \
	static.req static.req.files \
	symlinks.req symlinks.req.files \
	verify-elf

installprefix = $(DESTDIR)

all:

configdir = ${prefix}/lib/rpm
config_DATA = .provides.sh 0common-files.req.list
config_SCRIPTS = \
	posttrans-filetriggers 0ldconfig.filetrigger \
	debuginfo.req debuginfo.req.files debuginfo.prov debuginfo.prov.files \
	functions find-package \
	find-debuginfo-files \
	find-scriptlet-requires \
	brp-adjust_libraries brp-alt brp-bytecompile_python \
	brp-cleanup brp-compress brp-debuginfo \
	brp-fix-perms brp-fixup brp-strip \
	brp-verify_elf brp-verify-info \
	compress_files \
	cpp.req cpp.req.files \
	find-lang \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	files.req files.req.files \
	ldd lib.req lib.req.files lib.prov lib.prov.files shlib.req.awk \
	pam.req pam.req.files pam.prov pam.prov.files \
	pkgconfig.req pkgconfiglib.req pkgconfig.req.files \
	rpmlib.req rpmlib.req.files \
	rpm2cpio.sh \
	shebang.req shebang.req.files \
	shell.req shell.req.files shell.prov shell.prov.files \
	strip_files \
	static.req static.req.files \
	symlinks.req symlinks.req.files \
	verify-elf

install-data-local:
	@LN_S@ pkgconfig.req $(DESTDIR)$(configdir)/pkgconfig.prov
	@LN_S@ pkgconfig.req.files $(DESTDIR)$(configdir)/pkgconfig.prov.files
	@LN_S@ pkgconfig.req.files $(DESTDIR)$(configdir)/pkgconfiglib.req.files
