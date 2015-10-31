// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef PARSE_H
#define PARSE_H

struct Expression;

struct ParseResult {
	int chars_read;
	struct Expression *expr;
	const char *error_msg;
};

// Parses a string as an s-expression (an expression that contains only cons
// cells and symbols). If the string cannot be parsed, stores NULL in the expr
// field and provides an error message.
struct ParseResult parse(const char *text);

#endif
