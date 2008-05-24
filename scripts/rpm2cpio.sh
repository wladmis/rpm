#!/bin/sh

pkg=$1
if [ "$pkg" = "" -o ! -e "$pkg" ]; then
    echo "no package supplied" 1>&2
   exit 1
fi

leadsize=96
o=`expr $leadsize + 8`
set `od -j $o -N 8 -t u1 $pkg`
il=`expr 256 \* \( 256 \* \( 256 \* $2 + $3 \) + $4 \) + $5`
dl=`expr 256 \* \( 256 \* \( 256 \* $6 + $7 \) + $8 \) + $9`
# echo "sig il: $il dl: $dl"

sigsize=`expr 8 + 16 \* $il + $dl`
o=`expr $o + $sigsize + \( 8 - \( $sigsize \% 8 \) \) \% 8 + 8`
set `od -j $o -N 8 -t u1 $pkg`
il=`expr 256 \* \( 256 \* \( 256 \* $2 + $3 \) + $4 \) + $5`
dl=`expr 256 \* \( 256 \* \( 256 \* $6 + $7 \) + $8 \) + $9`
# echo "hdr il: $il dl: $dl"

hdrsize=`expr 8 + 16 \* $il + $dl`
o=`expr $o + $hdrsize`

magic=`dd if="$pkg" ibs=$o skip=1 count=1 2>/dev/null | dd bs=3 count=1 2>/dev/null`
gzip_magic=`printf '\037\213'`

case "$magic" in
	BZh) filter=bunzip2 ;;
	"$gzip_magic"?) filter=gunzip ;;
	# plain cpio
	070) filter=cat ;;
	# no magic in old lzma format
	*) filter=unlzma ;;
esac

dd if=$pkg ibs=$o skip=1 2>/dev/null | $filter
