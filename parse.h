// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

struct ParseResult {
	int chars_read;
	struct Expression expr;
	const char *err_msg;
};

// Parses a string as an s-expression that contains only pairs, symbols, and
// numbers. If the string cannot be parsed, provides an error message.
struct ParseResult parse(const char *text);

// Given a parse error message, returns true if the parse failed because it
// needed more input. Returns false if the parse would always fail, no matter
// what extra input was added.
bool more_input(const char *err_msg);

#endif
