// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "parse.h"

#include "error.h"
#include "intern.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>

// Attempts to parse a string of 'n' characters as an integer. Does not require
// a null terminator. On success, stores the integer in 'result' and returns
// true. Otherwise, returns false.
static bool parse_int(const char *s, size_t n, int *result) {
	int val = 0;
	int sign = 1;
	bool leading_zero = true;
	for (size_t i = 0; i < n; i++) {
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

// Parses the text of 'n' characters beginning at at 's' as a string expression
// (dynamically allocated) Handles backslash escapes.
static struct Expression parse_string(const char *s, size_t n) {
	if (n == 0) {
		return new_string(NULL, 0);
	}

	char* buf = xmalloc(n);
	char* ptr = buf;
	for (size_t i = 0; i < n; i++) {
		char c = s[i];
		if (s[i-1] == '\\') {
			switch (c) {
			case 'n':
				*ptr++ = '\n';
				break;
			case 'r':
				*ptr++ = '\r';
				break;
			case 't':
				*ptr++ = '\t';
				break;
			default:
				*ptr++ = c;
				break;
			}
		} else if (c != '\\') {
			*ptr++ = c;
		}
	}

	return new_string(buf, (size_t)(ptr - buf));
}

// Returns the number of leading whitespace characters in 'text'. Comments,
// which go from a semicolon to the end of the line, are treated as whitespace.
static size_t skip_whitespace(const char *text) {
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
	return (size_t)(s - text);
}

// Returns the number of symbol characters at the beginning of 'text'.
static size_t skip_symbol(const char *text) {
	const char *s = text;
	while (*s && *s != ';' && *s != '(' && *s != ')' && !isspace(*s)) {
		s++;
	}
	return (size_t)(s - text);
}

// Returns the number of string characters at the beginning of 'text'. Assumes
// 'text[-1]' is a double quote. The string continues until (but not including)
// the next double quote character that is not escaped by a backslash.
static size_t skip_string(const char* text) {
	const char *s = text;
	while (*s && !(s[-1] != '\\' && *s == '"')) {
		s++;
	}
	return (size_t)(s - text);
}

// Parses a pair, assuming the opening left parenthesis has already been read.
static struct ParseResult parse_pair(const char *text) {
	struct ParseResult result;
	result.err_type = -1;
	const char *s = text;
	s += skip_whitespace(s);

	if (*s == ')') {
		s++;
		result.expr = new_null();
		goto chars_read;
	}

	struct ParseResult first = parse(s);
	s += first.chars_read;
	if (first.err_type != -1) {
		result.err_type = first.err_type;
		goto chars_read;
	}

	if (*s == '.') {
		s++;
		struct ParseResult second = parse(s);
		s += second.chars_read;
		if (second.err_type != -1) {
			result.err_type = second.err_type;
			release_expression(first.expr);
			goto chars_read;
		}
		if (*s != ')') {
			result.err_type = *s ? ERR_EXPECTED_RPAREN : ERR_UNEXPECTED_EOI;
			release_expression(first.expr);
			goto chars_read;
		}
		s++;
		result.expr = new_pair(first.expr, second.expr);
	} else {
		struct ParseResult rest = parse_pair(s);
		s += rest.chars_read;
		if (rest.err_type != -1) {
			result.err_type = rest.err_type;
			release_expression(first.expr);
			goto chars_read;
		}
		result.expr = new_pair(first.expr, rest.expr);
	}

chars_read:
	result.chars_read = (size_t)(s - text);
	return result;
}

// Parses any expression.
struct ParseResult parse(const char *text) {
	struct ParseResult result;
	result.err_type = PARSE_SUCCESS;
	const char *s = text;
	s += skip_whitespace(s);

	size_t len;
	switch (*s) {
	case '\0':
		result.err_type = ERR_UNEXPECTED_EOI;
		break;
	case '(':
		s++;
		result = parse_pair(s);
		s += result.chars_read;
		break;
	case ')':
		result.err_type = ERR_UNEXPECTED_RPAREN;
		break;
	case '.':
		result.err_type = ERR_INVALID_DOT;
		break;
	case '#':
		len = skip_symbol(s + 1);
		if (len == 1 && s[1] == 't') {
			s += 2;
			result.expr = new_boolean(true);
		} else if (len == 1 && s[1] == 'f') {
			s += 2;
			result.expr = new_boolean(false);
		} else {
			result.err_type = ERR_INVALID_LITERAL;
		}
		break;
	case '\'':
		s++;
		result = parse(s);
		s += result.chars_read;
		if (result.err_type == PARSE_SUCCESS) {
			result.expr = new_pair(
					new_stdmacro(F_QUOTE),
					new_pair(result.expr, new_null()));
		}
		break;
	case '`':
		s++;
		result = parse(s);
		s += result.chars_read;
		if (result.err_type == PARSE_SUCCESS) {
			result.expr = new_pair(
					new_stdmacro(F_QUASIQUOTE),
					new_pair(result.expr, new_null()));
		}
		break;
	case ',':
		s++;
		enum StandardMacro stdmacro = F_UNQUOTE;
		if (*s == '@') {
			s++;
			stdmacro = F_UNQUOTE_SPLICING;
		}
		result = parse(s);
		s += result.chars_read;
		if (result.err_type == PARSE_SUCCESS) {
			result.expr = new_pair(
					new_stdmacro(stdmacro),
					new_pair(result.expr, new_null()));
		}
		break;
	case '"':
		s++;
		len = skip_string(s);
		if (!(s[len-1] == '\\' && (len < 2 || s[len-2] == '\\'))
				&& s[len] == '"') {
			result.expr = parse_string(s, len);
		} else {
			result.err_type = ERR_UNEXPECTED_EOI;
		}
		s += len;
		s++;
		break;
	default:;
		len = skip_symbol(s);
		assert(len > 0);
		int number;
		if (parse_int(s, len, &number)) {
			result.expr = new_number(number);
		} else {
			InternId symbol_id = intern_string_n(s, len);
			result.expr = new_symbol(symbol_id);
		}
		s += len;
		break;
	}

	if (result.err_type == PARSE_SUCCESS) {
		s += skip_whitespace(s);
		assert(s > text);
	}
	result.chars_read = (size_t)(s - text);
	return result;
}
