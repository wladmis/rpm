# Makefile for rpm scripts.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = \
	posttrans-filetriggers 0ldconfig.filetrigger \
	functions find-package \
	brp-adjust_libraries brp-alt brp-bytecompile_python \
	brp-cleanup brp-compress brp-fix-perms brp-fixup brp-strip brp-verify_elf \
	compress_files check-files \
	find-lang \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	files.req.list ldd \
	pam.prov pam.req pkgconfig.prov pkgconfig.req \
	rpm2cpio.sh \
	shell.req shell.prov shlib.req.awk strip_files \
	verify-elf

installprefix = $(DESTDIR)

all:

configdir = ${prefix}/lib/rpm
config_DATA = files.req.list
config_SCRIPTS = \
	posttrans-filetriggers 0ldconfig.filetrigger \
	functions find-package \
	brp-adjust_libraries brp-alt brp-bytecompile_python \
	brp-cleanup brp-compress brp-fix-perms brp-fixup brp-strip brp-verify_elf \
	compress_files check-files \
	find-lang \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	ldd \
	pam.prov pam.req pkgconfig.prov pkgconfig.req \
	rpm2cpio.sh \
	shell.req shell.prov shlib.req.awk strip_files \
	verify-elf
