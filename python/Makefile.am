# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

PYVER= @WITH_PYTHON_VERSION@

INCLUDES = -I. \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	-I/usr/include/python${PYVER} \
	@INCPATH@

noinst_HEADERS = db-py.h hash.h header-py.c header-py.h upgrade.h

mylibs= $(top_builddir)/rpmio/librpmio.la \
	$(top_builddir)/rpmdb/librpmdb.la \
	$(top_builddir)/lib/librpm.la

LDADD =

pythondir = $(libdir)/python${PYVER}/site-packages
python_LTLIBRARIES = rpmmodule.la

rpmmodule_la_LDFLAGS = -module -avoid-version
rpmmodule_la_LIBADD = $(mylibs) $(LIBS) -lpython${PYVER}
rpmmodule_la_DEPENDENCIES = $(mylibs)

rpmmodule_la_SOURCES = rpmmodule.c hash.c upgrade.c header-py.c db-py.c

.PHONY:	lclint
lclint:
	lclint $(DEFS) $(INCLUDES) $(rpmmodule_la_SOURCES)
