// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

struct ParseResult {
	size_t chars_read;
	struct Expression expr;
	const char *err_msg;
};

// Error message used when more input was expected.
extern const char *err_unexpected_eoi;

// Parses a string as an s-expression that contains only pairs, symbols, and
// numbers. If the string cannot be parsed, provides an error message.
struct ParseResult parse(const char *text);

#endif
