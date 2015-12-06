// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "parse.h"

#include "expr.h"
#include "intern.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Parse error messages.
const char *err_unexpected_eoi = "unexpected end of input";
static const char *err_expected_rparen = "expected character ')'";
static const char *err_unexpected_rparen = "unexpected character ')'";
static const char *err_improper_dot = "improperly placed dot";
static const char *err_invalid_literal = "invalid hash literal";

// Attempts to parse a string of n characters (does not require a null
// terminator) as an integer. On success, stores the integer in result and
// returns true. Otherwise, returns false.
static bool parse_int(const char *s, int n, int *result) {
	int val = 0;
	int sign = 1;
	bool leading_zero = true;
	for (int i = 0; i < n; i++) {
		char c = s[i];
		if (i == 0 && n > 1) {
			if (c == '+') {
				continue;
			}
			if (c == '-') {
				sign = -1;
				continue;
			}
		}
		if (c  < '0' || c > '9') {
			return false;
		}
		if (c != '0') {
			leading_zero = false;
		}
		if (!leading_zero) {
			val *= 10;
			val += c - '0';
		}
	}

	*result = sign * val;
	return true;
}

// Returns the number of leading whitespace characters in the text. Comments,
// which go from a semicolon to the end of the line, are treated as whitespace.
static int skip_whitespace(const char *text) {
	const char *s = text;
	bool comment = false;
	while (*s) {
		if (comment) {
			if (*s == '\n') {
				comment = false;
			}
		} else {
			if (*s == ';') {
				comment = true;
			} else if (!isspace(*s)) {
				break;
			}
		}
		s++;
	}
	return s - text;
}

// Returns the number of characters at the beginning of text that form a symbol.
static int skip_symbol(const char *text) {
	const char *s = text;
	while (*s && *s != ';' && *s != '(' && *s != ')' && !isspace(*s)) {
		s++;
	}
	return s - text;
}

// Parses a pair, assuming the opening '(' has already been read.
static struct ParseResult parse_pair(const char *text) {
	struct ParseResult result;
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
	if (first.err_msg) {
		result.err_msg = first.err_msg;
		goto END;
	}

	if (*s == '.') {
		s++;
		struct ParseResult second = parse(s);
		s += second.chars_read;
		if (second.err_msg) {
			result.err_msg = second.err_msg;
			release_expression(first.expr);
			goto END;
		}
		if (*s != ')') {
			result.err_msg = *s ? err_expected_rparen : err_unexpected_eoi;
			release_expression(first.expr);
			goto END;
		}
		s++;
		result.expr = new_pair(first.expr, second.expr);
		release_expression(first.expr);
		release_expression(second.expr);
	} else {
		struct ParseResult rest = parse_pair(s);
		s += rest.chars_read;
		if (rest.err_msg) {
			result.err_msg = rest.err_msg;
			release_expression(first.expr);
			goto END;
		}
		result.expr = new_pair(first.expr, rest.expr);
		release_expression(first.expr);
		release_expression(rest.expr);
	}

END:
	result.chars_read = s - text;
	return result;
}

// Parses any expression.
struct ParseResult parse(const char *text) {
	struct ParseResult result;
	result.err_msg = NULL;
	const char *s = text;
	s += skip_whitespace(s);

	switch (*s) {
	case '\0':
		result.err_msg = err_unexpected_eoi;
		break;
	case '(':
		s++;
		result = parse_pair(s);
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
		int number;
		if (parse_int(s, len, &number)) {
			result.expr = new_number(number);
		} else {
			InternID symbol_id = intern_string_n(s, len);
			result.expr = new_symbol(symbol_id);
		}
		s += len;
		break;
	}

	if (!result.err_msg) {
		s += skip_whitespace(s);
	}
	result.chars_read = s - text;
	return result;
}
