# Makefile for rpmbuild library.

AUTOMAKE_OPTIONS = 1.4 foreign

INCLUDES = -I. \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/rpmdb \
	-I$(top_srcdir)/rpmio \
	@INCPATH@

LIBS =

pkgincdir = $(pkgincludedir)
pkginc_HEADERS = rpmbuild.h rpmspec.h
noinst_HEADERS = buildio.h myftw.h

lib_LTLIBRARIES = librpmbuild.la
librpmbuild_la_SOURCES = \
	build.c expression.c files.c misc.c myftw.c names.c pack.c \
	parseBuildInstallClean.c parseChangelog.c parseDescription.c \
	parseFiles.c parsePreamble.c parsePrep.c parseReqs.c parseScript.c \
	parseSpec.c poptBT.c reqprov.c spec.c
librpmbuild_la_LDFLAGS = -release @VERSION@
librpmbuild_la_LIBADD = $(top_builddir)/lib/librpm.la
librpmbuild_la_DEPENDENCIES = $(top_builddir)/lib/librpm.la

.PHONY:	sources
sources:
	@echo $(librpmbuild_la_SOURCES:%=build/%)

.PHONY:	lclint
lclint:
	lclint ${DEFS} ${INCLUDES} ${librpmbuild_la_SOURCES}
