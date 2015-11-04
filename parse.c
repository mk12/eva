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
static const char *err_invalid_literal = "invalid hash literal";

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
	s += skip_whitespace(s);

	if (*s == ')') {
		s++;
		result.expr = new_null();
		goto END;
	}

	struct ParseResult first = parse(s);
	s += first.chars_read;
	if (!first.expr) {
		result.err_msg = first.err_msg;
		goto END;
	}

	if (*s == '.') {
		s++;
		struct ParseResult second = parse(s);
		s += second.chars_read;
		if (!second.expr) {
			result.err_msg = first.err_msg;
			goto END;
		}
		if (*s != ')') {
			result.err_msg = err_expected_rparen;
			goto END;
		}
		s++;
		result.expr = new_cons(first.expr, second.expr);
	} else {
		struct ParseResult rest = parse_cons(s);
		s += rest.chars_read;
		if (!rest.expr) {
			result.err_msg = rest.err_msg;
			goto END;
		}
		result.expr = new_cons(first.expr, rest.expr);
	}

END:
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
		result = parse_cons(s);
		s += result.chars_read;
		break;
	case ')':
		result.err_msg = err_unexpected_rparen;
		break;
	case '.':
		result.err_msg = err_improper_dot;
		break;
	case '#':
		if (*(s + 1) == 't') {
			s += 2;
			result.expr = new_boolean(true);
		} else if (*(s + 1) == 'f') {
			s += 2;
			result.expr = new_boolean(false);
		} else {
			result.err_msg = err_invalid_literal;
		}
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
