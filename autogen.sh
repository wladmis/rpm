#!/bin/sh -x

export CFLAGS
export LDFLAGS

: ${LIBTOOLIZE:=libtoolize}
: ${ACLOCAL:=aclocal}
: ${AUTOHEADER:=autoheader}
: ${AUTOMAKE:=automake}
: ${AUTOCONF:=autoconf}

$LIBTOOLIZE --version >/dev/null
$ACLOCAL --version >/dev/null
$AUTOHEADER --version >/dev/null
$AUTOMAKE --version >/dev/null
$AUTOCONF --version >/dev/null

gettextize --copy --force --quiet
cp /usr/share/gettext/intl/Makevars po/
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
