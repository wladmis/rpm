# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

INCLUDES = -I. \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/rpmio \
	@INCPATH@

DBLIBSRCS = db3.c db1.c falloc.c

LIBDB = @libdb3@

pkgincdir = $(pkgincludedir)
pkginc_HEADERS = rpmdb.h
noinst_HEADERS = falloc.h fprint.h rpmhash.h

LIBS =

lib_LTLIBRARIES = librpmdb.la
librpmdb_la_SOURCES = $(DBLIBSRCS) \
	dbconfig.c fprint.c merge.c poptDB.c rpmhash.c rpmdb.c
librpmdb_la_LDFLAGS = -release @VERSION@ @libdb3@
librpmdb_la_LIBADD = $(LIBDB) $(top_builddir)/lib/librpm.la
librpmdb_la_DEPENDENCIES = $(top_builddir)/lib/librpm.la

falloc.lo: falloc.c $(top_srcdir)/system.h $(top_srcdir)/rpmio/rpmio.h falloc.h
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

clean-local:
	rm -f $(DB3LOBJS) *.o

.PHONY:	sources
sources:
	@echo $(librpmdb_la_SOURCES:%=rpmdb/%)

.PHONY:	lclint
lclint:
	lclint $(DEFS) $(INCLUDES) $(librpmdb_la_SOURCES)

tdbi: librpmdb.la tdbi.o
	$(LINK) -all-static $@.o $< $(LIBS)

tfalloc: librpmdb.la tfalloc.o
	$(LINK) -all-static $@.o $< $(LIBS)
