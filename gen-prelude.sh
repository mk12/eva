#!/bin/bash

set -eufo pipefail

cat <<EOS > "$2"
// Copyright 2022 Mitchell Kember. Subject to the MIT License.

#include "prelude.h"

const char *const prelude_source =
$(sed -e '/^;.*$/d;/^$/d;s/\\/\\\\/g;s/"/\\"/g;s/^/    "/;s/$/"/;$s/$/;/' \
	< "$1")
EOS
