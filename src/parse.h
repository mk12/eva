// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

// ParseResult contains the result of parsing text. If 'err' is not NULL, then
// 'expr' will have a meaningful value.
struct ParseResult {
	size_t chars_read;
	struct Expression expr;
	struct ParseError *err;
};

// Parses a string as an s-expression of pairs, symbols, and numbers. If the
// string cannot be parsed, allocates and returns an error.
struct ParseResult parse(const char *text);

#endif
