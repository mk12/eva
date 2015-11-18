// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

struct ParseResult {
	int chars_read;
	struct Expression expr;
	const char *err_msg;
};

// Parses a string as an s-expression (an expression that contains only pairs
// and symbols). If the string cannot be parsed, stores NULL in the expr field
// and provides an error message.
struct ParseResult parse(const char *text);

#endif
