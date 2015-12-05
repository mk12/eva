#!/bin/bash

files="main.c expr.c env.c intern.c parse.c eval.c repl.c"
link="-lreadline"
binary="eva"

if [[ $1 == "-d" ]]; then
	cc -g $link $files -o $binary
else
	cc -Os $link $files -o $binary
fi
