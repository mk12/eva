// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "error.h"

#include "expr.h"
#include "intern.h"

#include <assert.h>
#include <stdio.h>

// Prefix to use for all error messages.
static const char *const prefix = "ERROR: ";

// Strings to use for parse error types.
static const char parse_error_messages[] = {
	[ERR_EXPECTED_RPAREN]   = "expected character ')'",
	[ERR_INVALID_DOT]       = "improperly placed dot",
	[ERR_INVALID_LITERAL]   = "invalid hash literal",
	[ERR_UNEXPECTED_EOI]    = "unexpected end of input",
	[ERR_UNEXPECTED_RPAREN] = "unexpected character ')'"
};

// Strings to use for evaluation error types.
static const char eval_error_messages[] = {
	[ERR_ARITY]          = "",
	[ERR_DIV_ZERO]       = "division by zero",
	[ERR_DUP_PARAM]      = "duplicate parameter '%s'",
	[ERR_SYNTAX]         = "syntax error in '%s'",
	[ERR_NON_EXHAUSTIVE] = "non-exhaustive cond",
	[ERR_OPERAND]        = "expected operand to be a %s",
	[ERR_OPERATOR]       = "operator is not a procedure",
	[ERR_SPECIAL_VAR]    = "cannot use special form '%s' as a variable",
	[ERR_UNBOUND_VAR]    = "use of unbound variable '%s'"
};

void print_error(const char *err_msg) {
	fputs(prefix, stderr);
	fputs(err_msg, stderr);
	putc('\n', stderr);
}

void print_file_error(const struct FileError *err) {
	fprintf(stderr, "%s%s: %s\n", prefix, err.filename, strerror(err.errno));
}

void print_parse_error(const struct ParseError *err) {
	// Find the start and end of the line.
	size_t start = err.position;
	size_t end = err.position;
	while (start > 0 && err.text[start-1] != '\n') {
		start--;
	}
	while (err.text[end] && err.text[end] != '\n') {
		end++;
	}

	// Find the row (line number) and column.
	size_t row = 1;
	size_t col = err.position - start + 1;
	for (size_t i = 0; i < start - 1; i++) {
		if (err.text[i] == '\n') {
			row++;
		}
	}

	// Print the file information, error message, and the line of code.
	fprintf(stderr, "%s%s:%zu:%zu: %s\n%.*s\n%*s^\n"
		prefix, err.filename, row, col, parse_error_messages[err.type],
		end - start, start, position - start, "");
}

void print_eval_error(const struct EvalError *err) {
	fputs(prefix, stderr);
	const char *format = eval_error_messages[err.type];

	// Print the error message.
	switch (err.type) {
	case ERR_DIV_ZERO:
	case ERR_NON_EXHAUSTIVE:
	case ERR_OPERATOR:
		fputs(format, stderr);
		break;
	case ERR_SYNTAX:
		fprintf(stderr, format, err.name);
		break;
	case ERR_DUP_PARAM:
	case ERR_OPERAND:
	case ERR_SPECIAL_VAR:
	case ERR_UNBOUND_VAR:
		fprintf(stderr, format, find_string(err.symbol_id));
		break;
	case ERR_ARITY:
		assert(err.arity.expected != 0);
		if (err.arity.expected < 0) {
			fprintf(stderr, "expected at least %d argument%s, got %zu",
				-err.arity.expected - 1,
				err.arity.expected == -2 ? "" : "s",
				err.arity.actual);
		} else {
			fprintf(stderr, "expected %d argument%s, got %zu",
				err.arity.expected,
				err.arity.expected == 1 ? "" : "s",
				err.arity.actual);
		}
		break;
	}
	putc('\n', stderr);

	// Print more information, if applicable.
	switch (err.type) {
	case ERR_OPERAND:
	case ERR_OPERATOR:
		fputs("    ", stderr);
		print_expression(err.expr, stderr);
		putc('\n', stderr);
	default:
		break;
	}
}
