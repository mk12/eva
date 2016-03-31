// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "error.h"
#include "expr.h"

// ParseResult contains the result of parsing text. The 'expr' field has a
// meaningful value if and only if 'err_type' is -1.
struct ParseResult {
	size_t chars_read;      // number of characters read
	struct Expression expr; // parsed expression
	int err_type;           // -1 or a ParseErrorType value
};

// Parses a string as an s-expression of pairs, symbols, and numbers. On
// success, returns the parse result with 'err_type' set to -1. Otherwise,
// returns a ParseErrorType in the result.
struct ParseResult parse(const char *text);

#endif
