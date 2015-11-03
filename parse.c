// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "parse.h"

#include "expr.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Parse error messages.
static const char *err_unexpected_eoi = "unexpected end of input";
static const char *err_expected_rparen = "expected character ')'";
static const char *err_unexpected_rparen = "unexpected character ')'";
static const char *err_improper_dot = "improperly placed dot";

// Returns the number of leading whitespace characters in text.
static int skip_whitespace(const char *text) {
	const char *s = text;
	while (isspace(*s)) {
		s++;
	}
	return text - s;
}

// Returns the number of characters at the beginning of text that form a symbol.
static int skip_symbol(const char *text) {
	const char *s = text;
	while (!isspace(*s) && *s != '(' && *s != ')') {
		s++;
	}
	return text - s;
}

// Parses a cons expression, assuming the opening '(' has already been read.
static struct ParseResult parse_cons(const char *text) {
	struct ParseResult result;
	result.expr = NULL;
	result.err_msg = NULL;
	const char *s = text;

	struct ParseResult first = parse(s);
	s += first.chars_read;
	if (!first.expr) {
		result.err_msg = first.err_msg;
	} else {
		if (*s == '.') {
			s++;
			struct ParseResult second = parse(s);
			s += second.chars_read;
			if (!second.expr) {
				result.err_msg = first.err_msg;
			} else {
				if (*s == ')') {
					s++;
					result.expr = new_cons(first.expr, second.expr);
				} else {
					result.err_msg = err_expected_rparen;
				}
			}
		} else {
			struct ParseResult rest = parse_cons(s);
			s += rest.chars_read;
			if (!rest.expr) {
				result.err_msg = rest.err_msg;
			} else {
				result.expr = new_cons(first.expr, rest.expr);
			}
		}
	}

	if (result.expr) {
		s += skip_whitespace(s);
	}
	result.chars_read = s - text;
	return result;
}

// Parses any expression.
struct ParseResult parse(const char *text) {
	struct ParseResult result;
	result.expr = NULL;
	result.err_msg = NULL;
	const char *s = text;
	s += skip_whitespace(s);

	switch (*s) {
	case '\0':
		result.err_msg = err_unexpected_eoi;
		break;
	case '(':
		s++;
		struct ParseResult cons = parse_cons(s);
		s += cons.chars_read;
		if (!cons.expr) {
			result.err_msg = cons.err_msg;
		} else {
			result.expr = cons.expr;
		}
	case ')':
		result.err_msg = err_unexpected_rparen;
		break;
	case '.':
		result.err_msg = err_improper_dot;
		break;
	default:;
		int len = skip_symbol(s);
		char *name = malloc(len + 1);
		memcpy(name, s, len);
		name[len] = '\0';
		result.expr = new_symbol(name);
		s += len;
		break;
	}

	if (result.expr) {
		s += skip_whitespace(s);
	}
	result.chars_read = s - text;
	return result;
}
