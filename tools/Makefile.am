# Makefile for rpm tools.

AUTOMAKE_OPTIONS = 1.4 foreign

INCLUDES = -I. \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	@INCPATH@ \
	-I$(top_srcdir)/misc

EXTRA_DIST =	rpmchecksig.c

EXTRA_PROGRAMS = rpminject rpmsort

#myLDFLAGS= -L$(top_builddir)/build -L$(top_builddir)/lib \
#	-L$(top_builddir)/rpmio

myLDADD = \
	$(top_builddir)/build/librpmbuild.la \
	$(top_builddir)/lib/librpm.la \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(top_builddir)/rpmio/librpmio.la \
	@LIBINTL@

noinst_PROGRAMS = \
	dump dumpdb rpmarchive rpmheader rpmlead rpmsignature

pkgbindir = @RPMCONFIGDIR@
pkgbin_PROGRAMS = javadeps filesize relative pdeath_execute

rpmsort_SOURCES = rpmsort.c
rpmsort_LDFLAGS = @LDFLAGS_STATIC@
rpmsort_LDADD =	$(myLDADD)

javadeps_SOURCES = javadeps.c
javadeps_LDADD = $(myLDADD)

filesize_SOURCES = filesize.c

relative_SOURCES = relative.c

pdeath_execute_SOURCES = pdeath_execute.c

$(PROGRAMS): $(LDADD)

gnash.o: gnash.c
	$(COMPILE) -o $@ -c gnash.c

gnash: gnash.o
	$(LINK) -all-static -o $@ gnash.o $(myLDADD)

