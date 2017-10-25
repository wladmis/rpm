# Makefile for rpm scripts.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = \
	posttrans-filetriggers 0ldconfig.filetrigger \
	debuginfo.req debuginfo.req.files debuginfo.prov debuginfo.prov.files \
	functions find-package .provides.sh \
	find-debuginfo-files \
	find-scriptlet-requires \
	brp-adjust_libraries brp-alt brp-bytecompile_python \
	brp-check_contents brp-cleanup brp-compress brp-debuginfo \
	brp-fix-perms brp-fixup brp-hardlink_pyo_pyc \
	brp-verify_elf brp-verify-info \
	compress_files \
	cpp.req cpp.req.files \
	find-lang \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	fixup-desktop fixup-desktop.awk \
	files.req files.req.files 0common-files.req.list \
	is_elf_so_executable \
	ldd lib.req lib.req.files lib.prov lib.prov.files shlib.req.awk \
	pam.req pam.req.files pam.prov pam.prov.files \
	percolate \
	pkgconfig.req pkgconfiglib.req pkgconfig.req.files \
	rpmlib.req rpmlib.req.files \
	rpm2cpio.sh \
	shebang.req shebang.req.files \
	shell.req shell.req.files shell.prov shell.prov.files \
	static.req static.req.files \
	symlinks.req symlinks.req.files \
	tmpdir.sh \
	verify-elf

installprefix = $(DESTDIR)

all:

configdir = ${prefix}/lib/rpm
config_DATA = 0common-files.req.list # .provides.sh
config_SCRIPTS = \
	debuginfo.req debuginfo.req.files debuginfo.prov debuginfo.prov.files \
	find-debuginfo-files \
	find-scriptlet-requires \
	brp-alt \
	compress_files \
	cpp.req cpp.req.files \
	find-lang \
	fixup-binconfig fixup-pkgconfig fixup-libtool fixup-libraries \
	fixup-desktop fixup-desktop.awk \
	files.req files.req.files \
	is_elf_so_executable \
	ldd lib.req lib.req.files lib.prov lib.prov.files shlib.req.awk \
	pam.req pam.req.files pam.prov pam.prov.files \
	pkgconfig.req pkgconfiglib.req pkgconfig.req.files \
	percolate \
	rpmlib.req rpmlib.req.files \
	rpm2cpio.sh \
	shebang.req shebang.req.files \
	shell.req shell.req.files shell.prov shell.prov.files \
	static.req static.req.files \
	symlinks.req symlinks.req.files \
	tmpdir.sh \
	verify-elf \
	# 0ldconfig.filetrigger find-package functions posttrans-filetriggers

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(configdir)/brp.d
	$(INSTALL) -m755 brp-cleanup $(DESTDIR)$(configdir)/brp.d/008-cleanup.brp
	$(INSTALL) -m755 brp-fix-perms $(DESTDIR)$(configdir)/brp.d/016-fix-perms.brp
	$(INSTALL) -m755 brp-fixup $(DESTDIR)$(configdir)/brp.d/024-fixup.brp
	$(INSTALL) -m755 brp-check_contents $(DESTDIR)$(configdir)/brp.d/028-check_contents.brp
	$(INSTALL) -m755 brp-compress $(DESTDIR)$(configdir)/brp.d/032-compress.brp
	$(INSTALL) -m755 brp-verify-info $(DESTDIR)$(configdir)/brp.d/040-verify-info.brp
	$(INSTALL) -m755 brp-adjust_libraries $(DESTDIR)$(configdir)/brp.d/048-adjust_libraries.brp
	$(INSTALL) -m755 brp-debuginfo $(DESTDIR)$(configdir)/brp.d/056-debuginfo.brp
	$(INSTALL) -m755 brp-verify_elf $(DESTDIR)$(configdir)/brp.d/064-verify_elf.brp
	$(INSTALL) -m755 brp-bytecompile_python $(DESTDIR)$(configdir)/brp.d/072-bytecompile_python.brp
	$(INSTALL) -m755 brp-hardlink_pyo_pyc $(DESTDIR)$(configdir)/brp.d/128-hardlink_pyo_pyc.brp
	@LN_S@ pkgconfig.req $(DESTDIR)$(configdir)/pkgconfig.prov
	@LN_S@ pkgconfig.req.files $(DESTDIR)$(configdir)/pkgconfig.prov.files
	@LN_S@ pkgconfig.req.files $(DESTDIR)$(configdir)/pkgconfiglib.req.files
