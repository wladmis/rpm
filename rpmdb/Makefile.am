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
pkginc_HEADERS = hdrinline.h rpmdb.h
noinst_HEADERS = falloc.h fprint.h rpmhash.h

mylibpaths = -L$(top_builddir)/lib/.libs
mylibs = -lrpm

LIBS =

lib_LTLIBRARIES = librpmdb.la
librpmdb_la_SOURCES = $(DBLIBSRCS) \
	dbconfig.c header.c fprint.c merge.c poptDB.c rpmhash.c rpmdb.c \
	rpmvercmp.c tagname.c tagtbl.c
librpmdb_la_LDFLAGS = -release @VERSION@ @libdb3@
librpmdb_la_LIBADD = $(LIBDB) $(top_builddir)/rpmio/librpmio.la
librpmdb_la_DEPENDENCIES = $(top_builddir)/rpmio/librpmio.la

tagtbl.c: Makefile.am $(top_srcdir)/lib/rpmlib.h 
	@echo '/*@-bounds@*/' > $@
	@echo '#include "system.h"' >> $@
	@echo '#include <rpmlib.h>' >> $@
	@echo '' >> $@
	@echo '/*@observer@*/ /*@unchecked@*/' >> $@
	@echo 'static const struct headerTagTableEntry_s rpmTagTbl[] = {' >> $@
	${AWK} '/(RPMTAG_[A-Z0-9]*)[ \t]+([0-9]*)/ && !/internal/ { tt = "NULL"; if ($$5 == "c") {tt = "CHAR"} if ($$5 == "h") {tt = "INT16"} if ($$5 == "i") {tt = "INT32"} if ($$5 == "l") {tt = "INT64"} if ($$5 == "s") {tt = "STRING"} if ($$5 == "x") {tt = "BIN"} if ($$5 == "s[]") {tt = "STRING_ARRAY"} if ($$5 == "s{}") {tt = "I18NSTRING"} if ($$2 == "=") { printf("\t{ \"%s\",	%s	RPM_%s_TYPE },\n", $$1, $$3, tt) } else { printf("\t{ \"%s\",	%s,	RPM_%s_TYPE },\n", $$2, $$3, tt) } }' < $(top_srcdir)/lib/rpmlib.h >> $@
	@echo '	{ NULL, 0, 0 }' >> $@
	@echo '};' >> $@
	@echo '' >> $@
	@echo '/*@observer@*/ /*@unchecked@*/' >> $@
	@echo 'const struct headerTagTableEntry_s * rpmTagTable = rpmTagTbl;' >> $@
	@echo '' >> $@
	@echo '/*@unchecked@*/' >> $@
	@echo 'const int rpmTagTableSize = sizeof(rpmTagTbl) / sizeof(rpmTagTbl[0]) - 1;' >> $@
	@echo '/*@=bounds@*/' >> $@

BUILT_SOURCES = tagtbl.c

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
	$(LINK) -all-static $@.o $< $(mylibpaths) $(mylibs) $(LIBS)

tfalloc: librpmdb.la tfalloc.o
	$(LINK) -all-static $@.o $< $(mylibpaths) $(mylibs) $(LIBS)
