#!/bin/bash

cd "$(dirname "$0")"

name=$(basename "$0")
usage="usage: $name [-h] [-d]"
debug_opts="-DNDEBUG -Os"
output="eva"
cc=${CC:-clang}

while getopts ":do:h" opt; do
	case "${opt}" in
		d) debug_opts="-g";;
		o) output="$OPTARG";;
		h)
			echo "$usage"
			exit 0
			;;
		\?)
			echo "$name: $OPTARG: illegal option" >&2
			echo "$usage" >&2
			exit 1
			;;
	esac
done
shift $((OPTIND-1))

if (($# > 0)); then
	echo "$usage" >&2
	exit 1
fi

$cc -o $output -std=c11 $debug_opts \
	-Weverything -pedantic \
	-lreadline \
	*.c
