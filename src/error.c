// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "error.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Prefix to use for all error messages.
static const char *const prefix = "ERROR: ";

// Filename to use when input is from standard input.
static const char *const stdin_filename = "<stdin>";

// Expression type names used for type errors.
static const char *expr_type_names[] = {
	[E_NULL]    = "NULL",
	[E_SYMBOL]  = "SYMBOL",
	[E_NUMBER]  = "NUMBER",
	[E_BOOLEAN] = "BOOLEAN",
	[E_SPECIAL] = "SPECIAL",
	[E_PAIR]    = "PAIR",
	[E_LAMBDA]  = "LAMBDA"
};

// Strings to use for parse error types.
static const char *parse_error_messages[] = {
	[ERR_EXPECTED_RPAREN]   = "expected character ')'",
	[ERR_INVALID_DOT]       = "improperly placed dot",
	[ERR_INVALID_LITERAL]   = "invalid hash literal",
	[ERR_UNEXPECTED_EOI]    = "unexpected end of input",
	[ERR_UNEXPECTED_RPAREN] = "unexpected character ')'"
};

// Strings to use for evaluation error types.
static const char *eval_error_messages[] = {
	[ERR_ARITY]          = "",
	[ERR_DIV_ZERO]       = "division by zero",
	[ERR_DUP_PARAM]      = "duplicate parameter '%s'",
	[ERR_INVALID_VAR]    = "cannot use special form '%s' as a variable",
	[ERR_NON_EXHAUSTIVE] = "non-exhaustive cond expression",
	[ERR_OP_NOT_PROC]    = "operator is not a procedure",
	[ERR_PROC_CALL]      = "malformed procedure call",
	[ERR_READ]           = "",
	[ERR_SYNTAX]         = "syntax error in special form '%s'",
	[ERR_TYPE]           = "(argument %zu) %s expected",
	[ERR_UNBOUND_VAR]    = "use of unbound variable '%s'"
};

struct ParseError *new_parse_error(
		enum ParseError type, char *text, size_t index, bool owns_text) {
	struct ParseError *err = malloc(sizeof *err);
	err->type = type;
	err->text = text;
	err->index = index;
	err->owns_text = owns_text;
	return err;
}

struct EvalError *new_eval_error(enum EvalErrorType type) {
	struct EvalError *err = malloc(sizeof *err);
	err->type = type;
	return err;
}

struct EvalError *new_eval_error_symbol(enum EvalErrorType type, InternId id) {
	struct EvalError *err = new_eval_error(type);
	err->symbol_id = id;
	return err;
}

struct EvalError *new_syntax_error(const char *str) {
	struct EvalError *err = new_eval_error(ERR_SYNTAX);
	err->str = str;
	return err;
}

struct EvalError *new_type_error(
		enum ExpressionType expected_type,
		size_t arg_pos,
		struct Expression expr) {
	struct EvalError *err = new_eval_error(ERR_TYPE);
	err->expected_type = expected_type;
	err->arg_pos = arg_pos;
	err->expr = retain_expression(expr);
	return err;
}

void free_parse_error(struct ParseError *err) {
	if (err->owns_text) {
		free(err->text);
	}
	free(err);
}

void free_eval_error(struct EvalError *err) {
	switch (err->type) {
	case ERR_OP_NOT_PROC:
	case ERR_TYPE:
		release_expression(err->expr);
		break;
	case ERR_READ:
		free_parse_error(err->parse_err);
	default:
		break;
	}
	free(err);
}

void print_error(const char *err_msg) {
	fputs(prefix, stderr);
	fputs(err_msg, stderr);
	putc('\n', stderr);
}

void print_file_error(const char *filename) {
	fprintf(stderr, "%s%s: %s\n", prefix, filename, strerror(errno));
}

void print_parse_error(const char *filename, const struct ParseError *err) {
	// Find the start and end of the line.
	size_t start = err->index;
	size_t end = err->index;
	while (start > 0 && err->text[start-1] != '\n') {
		start--;
	}
	while (err->text[end] && err->text[end] != '\n') {
		end++;
	}

	// Find the row (line number) and column.
	size_t row = 1;
	size_t col = err->index - start + 1;
	for (size_t i = 0; i < start - 1; i++) {
		if (err->text[i] == '\n') {
			row++;
		}
	}

	// Print the file information, error message, and the line of code.
	fprintf(stderr, "%s%s:%zu:%zu: %s\n%.*s\n%*s^\n",
		prefix, filename, row, col, parse_error_messages[err->type],
		(int)(end - start), err->text + start,
		(int)(err->index - start), "");
}

void print_eval_error(const struct EvalError *err) {
	fputs(prefix, stderr);
	const char *format = eval_error_messages[err->type];

	// Print the error message.
	switch (err->type) {
	case ERR_READ:
		print_parse_error(stdin_filenmame, err->parse_err);
		return;
	case ERR_DIV_ZERO:
	case ERR_NON_EXHAUSTIVE:
	case ERR_OP_NOT_PROC:
		fputs(format, stderr);
		break;
	case ERR_SYNTAX:
		fprintf(stderr, format, err->name);
		break;
	case ERR_DUP_PARAM:
	case ERR_SPECIAL_VAR:
	case ERR_UNBOUND_VAR:
		fprintf(stderr, format, find_string(err->symbol_id));
		break;
	case ERR_TYPE:
		fprintf(stderr, format, err->position, err->arg_pos,
			expr_type_names[err->type_wanted]);
		break;
	case ERR_ARITY:
		assert(err->arity != 0);
		if (err->arity >= 0) {
			fprintf(stderr, "expected %d argument%s, got %zu",
				err->arity, err->arity == 1 ? "" : "s", err->n_args);
		} else {
			fprintf(stderr, "expected at least %d argument%s, got %zu",
				-(err->arity + 1), err->arity == -2 ? "" : "s", err->n_args);
		}
		break;
	}
	putc('\n', stderr);

	// Print more information, if applicable.
	switch (err->type) {
	case ERR_OP_NOT_PROC:
	case ERR_TYPE:
		fputs("    ", stderr);
		print_expression(err->expr, stderr);
		putc('\n', stderr);
	default:
		break;
	}
}
