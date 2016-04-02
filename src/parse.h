// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

#define PARSE_SUCCESS (-1)

// ParseResult contains the result of parsing text. The 'expr' field has a
// meaningful value if and only if 'err_type' is -1.
struct ParseResult {
	size_t chars_read;      // number of characters read
	struct Expression expr; // parsed expression
	int err_type;           // PARSE_SUCCESS or a ParseErrorType value
};

// Parses a string as an s-expression of pairs, symbols, and numbers. On
// success, returns the parse result with 'err_type' set to PARSE_SUCCESS.
// Otherwise, returns a ParseErrorType in the result.
struct ParseResult parse(const char *text);

#endif
