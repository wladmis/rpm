#!/bin/sh

export CFLAGS
export LDFLAGS

: ${LIBTOOLIZE:=libtoolize}
: ${ACLOCAL:=aclocal}
: ${AUTOMAKE:=automake}
: ${AUTOCONF:=autoconf}
: ${AUTOHEADER:=autoheader}

LTV='libtoolize (GNU libtool) 1\.4.*'
ACV='autoconf (GNU Autoconf) 2\.5[3-9]'
AMV='automake (GNU automake) 1\.6\.[1-9]'
USAGE="
This script documents the versions of the tools I'm using to build rpm:
	libtool-1.4
	autoconf-2.53
	automake-1.6.1
Simply edit this script to change the libtool/autoconf/automake versions
checked if you need to, as rpm should build (and has built) with all
recent versions of libtool/autoconf/automake.
"

$LIBTOOLIZE --version |head -1 |grep -qs "$LTV" || { echo "$USAGE"; exit 1; }
$AUTOCONF --version |head -1 |grep -qs "$ACV" || { echo "$USAGE"; exit 1; }
$AUTOMAKE --version |head -1 |grep -qs "$AMV" || { echo "$USAGE"; exit 1; }

gettextize --copy --force --quiet
$LIBTOOLIZE --copy --force
$ACLOCAL -I m4
$AUTOHEADER
$AUTOMAKE -a -c
$AUTOCONF

if [ "$1" = "--noconfigure" ]; then 
    exit 0;
fi

if [ X"$@" = X  -a "X`uname -s`" = "XLinux" ]; then
    if [ -d /usr/share/man ]; then
	mandir=/usr/share/man
	infodir=/usr/share/info
    else
	mandir=/usr/man
	infodir=/usr/info
    fi
    ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --infodir=${infodir} --mandir=${mandir} "$@"
else
    ./configure "$@"
fi
