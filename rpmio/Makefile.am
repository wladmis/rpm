# Makefile for rpm library.

AUTOMAKE_OPTIONS = 1.4 foreign

EXTRA_DIST = tdigest.c tficl.c tkey.c trpmio.c

EXTRA_PROGRAMS = tdigest tkey tring trpmio dumpasn1

INCLUDES = -I. \
	@INCPATH@

pkgincdir = $(pkgincludedir)
pkginc_HEADERS = \
	rpmio.h rpmurl.h rpmmacro.h rpmlog.h rpmmessages.h rpmerr.h \
	ugid.h
noinst_HEADERS = rpmio_internal.h rpmpgp.h

lib_LTLIBRARIES = librpmio.la
librpmio_la_SOURCES = digest.c macro.c rpmio.c rpmlog.c rpmmalloc.c \
	rpmpgp.c rpmrpc.c strcasecmp.c stubs.c url.c ugid.c
librpmio_la_LDFLAGS = -release @VERSION@
librpmio_la_LIBADD = @LIBBEECRYPT@

# XXX Add internal libtool dependence
install-data-local:
	@cd $(DESTDIR)/$(libdir) && \
	sed -e 's,^\(dependency_libs=.*\) -lrt,\1,g;s,^\(dependency_libs=.*\) -lpthread,\1,g' < librpmio.la > .librpmio.la && \
	mv .librpmio.la librpmio.la

clean-local:
	rm -f *.o

tdigest_SOURCES = tdigest.c
tdigest_LDADD = librpmio.la -lpopt

trpmio_SOURCES = trpmio.c
trpmio_LDADD = librpmio.la -lpopt

tkey_SOURCES = tkey.c
tkey_LDFLAGS = -all-static
tkey_LDADD = librpmio.la -lpopt

tring_SOURCES = tring.c
tring_LDFLAGS = -all-static
tring_LDADD = librpmio.la -lpopt

dumpasn1_SOURCES = dumpasn1.c

.PHONY:	sources
sources:
	@echo $(librpmio_la_SOURCES:%=rpmio/%)

.PHONY:	lclint
lclint:
	lclint $(DEFS) $(INCLUDES) $(librpmio_la_SOURCES)

tficl.o: tficl.c
	$(COMPILE) -I/usr/include/ficl -o $@ -c tficl.c 

tficl: tficl.o
	$(LINK) -o $@ tficl.o -lficl
