// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "error.h"
#include "expr.h"

// ParseResult contains the result of parsing text. The 'expr' field has a
// meaningful value if and only if 'err_type' is -1.
struct ParseResult {
	size_t chars_read;
	struct Expression expr;
	enum ParseErrorType err_type;
};

// Parses a string as an s-expression of pairs, symbols, and numbers. On
// success, returns the parse result with 'err_type' set to -1. Otherwise,
// returns the error type in the parse result.
struct ParseResult parse(const char *text);

#endif
