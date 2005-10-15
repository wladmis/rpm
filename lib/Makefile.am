# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

INCLUDES = -I. \
	-I$(top_srcdir)/build \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	@INCPATH@

EXTRA_DIST = getdate.y

pkgincdir = $(pkgincludedir)
pkginc_HEADERS = \
	header.h misc.h rpmcli.h rpmlib.h stringbuf.h
noinst_HEADERS = \
	cpio.h depends.h fsm.h header_internal.h \
	manifest.h psm.h \
	rpmlead.h signature.h

mylibpaths = \
	-L$(top_builddir)/lib/.libs \
	-L$(top_builddir)/rpmdb/.libs \
	-L$(top_builddir)/rpmio/.libs

mylibs = -lrpm -lrpmdb -lrpmio @LIBS@ @LIBINTL@
LIBS =

lib_LTLIBRARIES = librpm.la
librpm_la_SOURCES = \
	closeall.c cpio.c depends.c formats.c fs.c fsm.c getdate.c \
	header_internal.c manifest.c misc.c package.c \
	problems.c poptI.c poptK.c poptQV.c psm.c query.c \
	rpmchecksig.c rpminstall.c rpmlead.c rpmlibprov.c rpmrc.c \
	signature.c stringbuf.c transaction.c \
	verify.c
librpm_la_LDFLAGS = -release @VERSION@
librpm_la_LIBADD = $(top_builddir)/rpmdb/librpmdb.la $(top_builddir)/rpmio/librpmio.la @LIBPOPT@
librpm_la_DEPENDENCIES = $(top_builddir)/rpmdb/librpmdb.la $(top_builddir)/rpmio/librpmio.la

getdate.c: getdate.y
	@echo expect 10 shift/reduce conflicts
	$(YACC) $(srcdir)/getdate.y
	-@if test -f y.tab.c; then \
	 { echo "/*@-globstate -statictrans -unqualifiedtrans -noparams @*/";\
	   echo "/*@-retvalint -usedef -varuse -nullderef -nullassign @*/";\
	   echo "/*@-readonlytrans -modunconnomods -compdef @*/";\
	   sed	-e 's,y.tab.c,getdate.c,' y.tab.c \
		-e 's,^YYSTYPE ,static &,' \
		-e 's,^short ,static &,' \
		-e 's,^const short ,static &,' \
		-e 's,^int yydebug,/*@unused@*/ static &,' \
		-e 's,^int ,static &,' ;\
	   echo "/*@=readonlytrans =modunconnomods =compdef @*/";\
	   echo "/*@=retvalint =usedef =varuse =nullderef =nullassign @*/";\
	   echo "/*@=globstate =statictrans =unqualifiedtrans =noparams @*/";\
	 } > getdate.c ;\
	   rm -f y.tab.c; \
	else \
	   if test -f getdate.tab.c ; then \
	       mv getdate.tab.c getdate.c ; \
	   else \
	       echo '*** Unable to create getdate.c' ;\
	   fi ;\
	fi

BUILT_SOURCES = getdate.c

.PHONY:	sources
sources:
	@echo $(librpm_la_SOURCES:%=lib/%)

.PHONY:	lclint
lclint:
	lclint $(DEFS) $(INCLUDES) $(librpm_la_SOURCES)

th: th.c librpm.la
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -o $@ $< $(mylibpaths) $(mylibs) $(LIBS)

trb: trb.o librpm.la
	$(LINK) @LDFLAGS_STATIC@ $(CFLAGS) $(DEFS) $(INCLUDES) -o $@ trb.o $(mylibs) $(LIBS)

trhn: trhn.o librpm.la
	$(LINK) @LDFLAGS_STATIC@ $(CFLAGS) $(DEFS) $(INCLUDES) -o $@ trhn.o $(mylibs) $(LIBS)
