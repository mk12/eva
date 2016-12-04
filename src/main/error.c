// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "error.h"

#include "util.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Extern constants.
const char *const argv_filename = "<argv>";
const char *const stdin_filename = "<stdin>";

// Prefix to use for all error messages.
static const char *const prefix = "ERROR";

// Identation used to align with text after "eva> " prompt.
static const char *const indentation = "     ";

// Strings to use for parse error types.
static const char *const parse_error_messages[N_PARSE_ERROR_TYPES] = {
	[ERR_EXPECTED_RPAREN]   = "Expected character ')'",
	[ERR_INVALID_DOT]       = "Improperly placed dot",
	[ERR_INVALID_LITERAL]   = "Invalid hash literal",
	[ERR_UNEXPECTED_EOI]    = "Unexpected end of input",
	[ERR_UNEXPECTED_RPAREN] = "Unexpected character ')'"
};

// Strings to use for evaluation error types.
static const char *const eval_error_messages[N_EVAL_ERROR_TYPES] = {
	[ERR_ARITY]          = NULL,
	[ERR_CUSTOM]         = NULL,
	[ERR_DEFINE]         = "Invalid use of 'define'",
	[ERR_DIV_ZERO]       = "Division by zero",
	[ERR_DUP_PARAM]      = "Duplicate parameter '%s'",
	[ERR_NON_EXHAUSTIVE] = "Non-exhaustive 'cond'",
	[ERR_READ]           = NULL,
	[ERR_SYNTAX]         = "Invalid syntax",
	[ERR_TYPE_OPERAND]   = "Argument %zu: Expected %s, got %s: ",
	[ERR_TYPE_OPERATOR]  = "Operator: Expected %s or %s, got %s: ",
	[ERR_TYPE_VAR]       = "Variable: Expected %s, got %s: ",
	[ERR_UNBOUND_VAR]    = "Use of unbound variable '%s'",
	[ERR_UNQUOTE]        = "Invalid use of 'unquote' or 'unquote-splicing'"
};

struct ParseError *new_parse_error(
		enum ParseErrorType type, char *owned_text, size_t index) {
	struct ParseError *err = xmalloc(sizeof *err);
	err->type = type;
	err->owned_text = owned_text;
	err->index = index;
	return err;
}

struct EvalError *new_eval_error(enum EvalErrorType type) {
	struct EvalError *err = xmalloc(sizeof *err);
	err->type = type;
	err->has_code = false;
	return err;
}

struct EvalError *new_eval_error_expr(
		enum EvalErrorType type, struct Expression expr) {
	struct EvalError *err = new_eval_error(type);
	err->expr = retain_expression(expr);
	return err;
}

struct EvalError *new_eval_error_symbol(
		enum EvalErrorType type, InternId symbol_id) {
	struct EvalError *err = new_eval_error(type);
	err->symbol_id = symbol_id;
	return err;
}

struct EvalError *new_arity_error(Arity arity, size_t n_args) {
	struct EvalError *err = new_eval_error(ERR_ARITY);
	err->arity = arity;
	err->n_args = n_args;
	return err;
}

struct EvalError *new_read_error(struct ParseError *parse_err) {
	struct EvalError *err = new_eval_error(ERR_READ);
	err->parse_err = parse_err;
	return err;
}

struct EvalError *new_syntax_error(struct Expression code) {
	return attach_code(new_eval_error(ERR_SYNTAX), code);
}

struct EvalError *new_type_error(
		enum ExpressionType expected_type,
		const struct Expression *args,
		size_t arg_pos) {
	struct EvalError *err = new_eval_error_expr(
			ERR_TYPE_OPERAND, args[arg_pos]);
	err->expected_type = expected_type;
	err->arg_pos = arg_pos;
	return err;
}

struct EvalError *attach_code(struct EvalError *err, struct Expression code) {
	if (err->type != ERR_READ) {
		err->has_code = true;
		err->code = retain_expression(code);
	}
	return err;
}

void free_parse_error(struct ParseError *err) {
	free(err->owned_text);
	free(err);
}

void free_eval_error(struct EvalError *err) {
	switch (err->type) {
	case ERR_CUSTOM:
		free_array(err->array);
		break;
	case ERR_READ:
		free_parse_error(err->parse_err);
		break;
	case ERR_TYPE_OPERAND:
	case ERR_TYPE_OPERATOR:
	case ERR_TYPE_VAR:
		release_expression(err->expr);
		break;
	default:
		break;
	}
	if (err->has_code) {
		release_expression(err->code);
	}
	free(err);
}

void print_error(const char *context, const char *err_msg) {
	fprintf(stderr, "%s: %s: %s\n", prefix, context, err_msg);
}

void print_file_error(const char *filename) {
	fprintf(stderr, "%s: %s: %s\n", prefix, filename, strerror(errno));
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
	for (size_t i = start; i-- > 0;) {
		if (err->text[i] == '\n') {
			row++;
		}
	}

	// Print the file information, error message, and the line of code.
	fprintf(stderr, "%s: %s:%zu:%zu: %s\n%s%.*s\n%s%*s^\n",
			prefix, filename, row, col, parse_error_messages[err->type],
			indentation,
			(int)(end - start), err->text + start,
			indentation,
			(int)(err->index - start), "");
}

void print_eval_error(const char *filename, const struct EvalError *err) {
	// Read errors are really parse errors.
	if (err->type == ERR_READ) {
		print_parse_error(stdin_filename, err->parse_err);
		return;
	}

	// Print the error message.
	fprintf(stderr, "%s: %s: ", prefix, filename);
	const char *format = eval_error_messages[err->type];
	switch (err->type) {
	case ERR_READ:
		assert(false);
		break;
	case ERR_CUSTOM:
		break;
	case ERR_DEFINE:
	case ERR_DIV_ZERO:
	case ERR_NON_EXHAUSTIVE:
	case ERR_SYNTAX:
	case ERR_UNQUOTE:
		fputs(format, stderr);
		break;
	case ERR_DUP_PARAM:
	case ERR_UNBOUND_VAR:
		fprintf(stderr, format, find_string(err->symbol_id));
		break;
	case ERR_TYPE_OPERAND:
		fprintf(stderr, format,
				err->arg_pos + 1,
				expression_type_name(err->expected_type),
				expression_type_name(err->expr.type));
		break;
	case ERR_TYPE_OPERATOR:
		fprintf(stderr, format,
				expression_type_name(E_MACRO),
				expression_type_name(E_PROCEDURE),
				expression_type_name(err->expr.type));
		break;
	case ERR_TYPE_VAR:
		fprintf(stderr, format,
				expression_type_name(E_SYMBOL),
				expression_type_name(err->expr.type));
		break;
	case ERR_ARITY:
		assert(err->arity != 0);
		if (err->arity >= 0) {
			fprintf(stderr, "Expected %d argument%s, got %zu",
					err->arity,
					err->arity == 1 ? "" : "s",
					err->n_args);
		} else {
			fprintf(stderr, "Expected at least %d argument%s, got %zu",
					ATLEAST(err->arity),
					err->arity == -2 ? "" : "s",
					err->n_args);
		}
		break;
	}

	// Print the expression, if applicable.
	switch (err->type) {
	case ERR_CUSTOM:
		for (size_t i = 0; i < err->array.size; i++) {
			if (i > 0) {
				putc(' ', stderr);
			}
			print_expression(err->array.exprs[i], stderr);
		}
		break;
	case ERR_TYPE_OPERAND:
	case ERR_TYPE_OPERATOR:
	case ERR_TYPE_VAR:
		print_expression(err->expr, stderr);
		break;
	default:
		break;
	}
	putc('\n', stderr);

	// Print the context of the error.
	if (err->has_code) {
		fputs(indentation, stderr);
		print_expression(err->code, stderr);
		putc('\n', stderr);
	}
}
