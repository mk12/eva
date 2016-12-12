#!/bin/bash

cd "$(dirname "$0")"

SRC=src/prelude.scm
DST=src/prelude.c

HEADER=prelude.h

echo '// Copyright 2016 Mitchell Kember. Subject to the MIT License.' > $DST
echo -e "\n#include \"$HEADER\"\n" >> $DST
echo 'const char *const prelude_source =' >> $DST
sed -e '/^;.*$/d;/^$/d;s/\\/\\\\/g;s/"/\\"/g;s/^/    "/;s/$/"/;$s/$/;/' \
	< $SRC >> $DST
