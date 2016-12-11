// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

#include <stddef.h>

// Constant indicating that a parse was successful.
#define PARSE_SUCCESS (-1)

// ParseResult contains the result of parsing text. The 'expr' field has a
// meaningful value if and only if 'err_type' is PARSE_SUCCESS.
struct ParseResult {
	size_t chars_read;      // number of characters read
	struct Expression expr; // parsed expression
	int err_type;           // PARSE_SUCCESS or a ParseErrorType value
};

// Parses a string as an s-expression of pairs, symbols, and numbers. On
// success, returns the parse result with 'err_type' set to PARSE_SUCCESS.
// Otherwise, returns a ParseErrorType in the result.
struct ParseResult parse(const char *text);

// Attempts to parse a string of 'n' characters as a number. Does not require
// a null terminator. On success, stores the integer in 'result' and returns
// true. Otherwise, returns false.
bool parse_number(const char *s, size_t n, Number *result);

#endif
