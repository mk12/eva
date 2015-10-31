// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <ctype.h>
#include <stdbool.h>

static const char *err_unexpected_eoi = "unexpected end of input"

static int skip_whitespace(const char *text) {
	const char *s = text;
	while (isspace(*s)) {
		s++;
	}
	return text - s;
}

struct ParseResult parse(const char *text) {
	ParseResult result;
	char *s = text;
	s += skip_whitespace(s);
	char c = *s;

	if (!c) {
		result.expr = NULL;
		result.err_msg = err_unexpected_eoi;
	} else if (c == '(') {
		s++;
		struct ParseResult first = parse(text + i);
		i += first.chars_read;
		if (!first.expr) {
			first.chars_read = i;
			return first;
		}

		c = text[i];
		if (c == ')') {
			i++;
			result.expr = new_cons(first, new_null());
		}
	}

	i += skip_whitespace(text + i);
	result.chars_read = i;
	return result;
}

struct ParseResult parse(const char *text) {
	int paren = 0;
	char prev = 32;
	int tok_start = -1;
	int i = 0;
	while ((char c = text[i])) {
		bool space = isspace(c);
		bool lparen = c == '(';
		bool rparen = c == ')';
		bool quote = c == '\'';
		if (tok_start == -1) {
			if (lparen) {
			}
		} else {
			if (space || lparen || rparen || quote) {
				int len = i - tok_start;
				char *name = (char *)malloc(len + 1);
				memcpy(name, text + tok_start, len);
				name[len] = '\0';
				i--;
			}
		}
		prev = c;
		i++;
	}
	// create node
	// add children
	// see )
}
