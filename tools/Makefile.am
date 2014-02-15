# Makefile for rpm tools.

AUTOMAKE_OPTIONS = 1.4 foreign

AM_CPPFLAGS = -I. \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	@INCPATH@ \
	-I$(top_srcdir)/misc

EXTRA_DIST =	rpmchecksig.c

EXTRA_PROGRAMS = rpminject

#myLDFLAGS= -L$(top_builddir)/build -L$(top_builddir)/lib \
#	-L$(top_builddir)/rpmio

LDADD = \
	$(top_builddir)/build/librpmbuild.la \
	$(top_builddir)/lib/librpm.la \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(top_builddir)/rpmio/librpmio.la \
	@LIBINTL@

noinst_PROGRAMS = \
	dump dumpdb rpmarchive rpmheader rpmlead rpmsignature setcmp.static

pkgbindir = @RPMCONFIGDIR@
pkgbin_PROGRAMS = javadeps filesize dump_ld_config relative pdeath_execute mkset setcmp debugedit
bin_PROGRAMS = rpmvercmp rpmevrcmp

javadeps_SOURCES = javadeps.c

filesize_SOURCES = filesize.c
filesize_LDADD =

relative_SOURCES = relative.c
relative_LDADD =

dump_ld_config_SOURCES = dump_ld_config.c
dump_ld_config_LDADD =

pdeath_execute_SOURCES = pdeath_execute.c
pdeath_execute_LDADD =

mkset_SOURCES = mkset.c
setcmp_SOURCES = setcmp.c

setcmp_static_SOURCES = setcmp.c
setcmp_static_LDADD = @LDFLAGS_STATIC@ $(LDADD)

debugedit_SOURCES = debugedit.c hashtab.c hashtab.h
debugedit_LDADD = @WITH_LIBELF_LIB@ @LIBBEECRYPT@ @LIBPOPT@

rpmvercmp_SOURCES = rpmvercmp.c

rpmevrcmp_SOURCES = rpmevrcmp.c

$(PROGRAMS): $(LDADD)

gnash.o: gnash.c
	$(COMPILE) -o $@ -c gnash.c

gnash: gnash.o
	$(LINK) -all-static -o $@ gnash.o $(LDADD)

