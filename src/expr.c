// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Change to 1 to enable reference count logging.
#define REF_COUNT_LOGGING 0

// Counters for the number of allocated boxes and the sum of all refernce
// counts. Used for debugging memory bugs.
#if REF_COUNT_LOGGING
static int total_box_count = 0;
static int total_ref_count = 0;
#endif

// Names and arities of special procedures.
static const struct {
	const char *name;
	int arity;
} special_procs[N_SPECIAL_TYPES] = {
	[S_EVAL]      = {"eval", 1},
	[S_APPLY]     = {"apply", 2},
	[S_NULL]      = {"null?", 1},
	[S_SYMBOL]    = {"symbol?", 1},
	[S_NUMBER]    = {"number?", 1},
	[S_BOOLEAN]   = {"boolean?", 1},
	[S_PAIR]      = {"pair?", 1},
	[S_PROCEDURE] = {"procedure?", 1},
	[S_EQ]        = {"eq?", 2},
	[S_NUM_EQ]    = {"=", 2},
	[S_LT]        = {"<", 2},
	[S_GT]        = {">", 2},
	[S_LE]        = {"<=", 2},
	[S_GE]        = {">=", 2},
	[S_CONS]      = {"cons", 2},
	[S_CAR]       = {"car", 1},
	[S_CDR]       = {"cdr", 1},
	[S_ADD]       = {"+", -1},
	[S_SUB]       = {"-", -2},
	[S_MUL]       = {"*", -1},
	[S_DIV]       = {"/", -2},
	[S_REM]       = {"remainder", 2},
	[S_NOT]       = {"not", 1},
	[S_READ]      = {"read", 0},
	[S_WRITE]     = {"write", 1}
};

int special_arity(enum SpecialType type) {
	return special_procs[type].arity;
}

const char *special_name(enum SpecialType type) {
	return special_procs[type].name;
}

struct Expression new_null(void) {
	return (struct Expression){ .type = E_NULL };
}

struct Expression new_symbol(InternId id) {
	return (struct Expression){ .type = E_SYMBOL, .symbol_id = id };
}

struct Expression new_number(long n) {
	return (struct Expression){ .type = E_NUMBER, .number = n };
}

struct Expression new_boolean(bool b) {
	return (struct Expression){ .type = E_BOOLEAN, .boolean = b };
}

struct Expression new_special(enum SpecialType type) {
	return (struct Expression){ .type = E_SPECIAL, .special_type = type };
}

// Logs information about reference counts to standard error.
#if REF_COUNT_LOGGING
static void log_ref_count(const char *action, struct Expression expr) {
	bool pair = expr.type == E_PAIR;
	assert(pair || expr.type == E_LAMBDA);
	fprintf(stderr, "[%d/%d] %7s %6s [%d] ",
			total_ref_count, total_box_count, action, pair ? "pair" : "lambda",
			expr.box->ref_count);
	if (pair) {
		putc('(', stderr);
		print_expression(expr.box->car, stderr);
		fputs(" . ", stderr);
		print_expression(expr.box->cdr, stderr);
	} else {
		fputs("(lambda (?) ", stderr);
		print_expression(expr.box->body, stderr);
	}
	fputs(")\n", stderr);
}
#endif

struct Expression new_pair(struct Expression car, struct Expression cdr) {
	struct Box *box = malloc(sizeof *box);
	box->ref_count = 1;
	box->car = car;
	box->cdr = cdr;
	struct Expression expr = { .type = E_PAIR, .box = box };
#if REF_COUNT_LOGGING
	total_box_count++;
	total_ref_count++;
	log_ref_count("create", expr);
#endif
	return expr;
}

struct Expression new_lambda(
		int arity, InternId *params, struct Expression body) {
	struct Box *box = malloc(sizeof *box);
	box->ref_count = 1;
	box->arity = arity;
	box->params = params;
	box->body = body;
	struct Expression expr = { .type = E_LAMBDA, .box = box };
#if REF_COUNT_LOGGING
	total_box_count++;
	total_ref_count++;
	log_ref_count("create", expr);
#endif
	return expr;
}

static void dealloc_expression(struct Expression expr) {
#if REF_COUNT_LOGGING
	if (expr.type == E_PAIR || expr.type == E_LAMBDA) {
		total_box_count--;
		log_ref_count("dealloc", expr);
	}
#endif
	// Free the expression's box and release sub-boxes.
	switch (expr.type) {
	case E_PAIR:
		release_expression(expr.box->car);
		release_expression(expr.box->cdr);
		free(expr.box);
		break;
	case E_LAMBDA:
		free(expr.box->params);
		release_expression(expr.box->body);
		free(expr.box);
		break;
	default:
		break;
	}
}

struct Expression retain_expression(struct Expression expr) {
	// Increase the reference count of the box.
	switch (expr.type) {
	case E_PAIR:
	case E_LAMBDA:
		expr.box->ref_count++;
#if REF_COUNT_LOGGING
		total_ref_count++;
		log_ref_count("retain", expr);
#endif
		break;
	default:
		break;
	}
	return expr;
}

void release_expression(struct Expression expr) {
	// Decrease the reference count of the box, and deallocate if it reaches 0.
	switch (expr.type) {
	case E_PAIR:
	case E_LAMBDA:
		expr.box->ref_count--;
#if REF_COUNT_LOGGING
		total_ref_count--;
		log_ref_count("release", expr);
#endif
		if (expr.box->ref_count <= 0) {
			dealloc_expression(expr);
		}
		break;
	default:
		break;
	}
}

// Prints a pair to the 'stream', assuming the left parenthesis has already been
// printed. Uses standard Lisp s-expression notation.
static void print_pair(struct Box *box, bool first, FILE *stream) {
	if (!first) {
		putchar(' ');
	}
	print_expression(box->car, stream);
	switch (box->cdr.type) {
	case E_NULL:
		putchar(')');
		break;
	case E_PAIR:
		print_pair(box->cdr.box, false, stream);
		break;
	default:
		// Print a dot before the last cdr if it is not null.
		fputs(" . ", stdout);
		print_expression(box->cdr, stream);
		putchar(')');
		break;
	}
}

bool expression_eq(struct Expression lhs, struct Expression rhs) {
	if (lhs.type != rhs.type) {
		return false;
	}
	switch (lhs.type) {
	case E_NULL:
		return true;
	case E_SYMBOL:
		return lhs.symbol_id == rhs.symbol_id;
	case E_NUMBER:
		return lhs.number == rhs.number;
	case E_BOOLEAN:
		return lhs.boolean == rhs.boolean;
	case E_SPECIAL:
		return lhs.special_type == rhs.special_type;
	case E_PAIR:
	case E_LAMBDA:
		return lhs.box == rhs.box;
	}
}

void print_expression(struct Expression expr, FILE *stream) {
	switch (expr.type) {
	case E_NULL:
		fputs("()", stream);
		break;
	case E_PAIR:
		putc('(', stream);
		print_pair(expr.box, true, stream);
		break;
	case E_SYMBOL:
		fputs(find_string(expr.symbol_id), stream);
		break;
	case E_NUMBER:
		fprintf(stream, "%ld", expr.number);
		break;
	case E_BOOLEAN:
		fprintf(stream, "#%c", expr.boolean ? 't' : 'f');
		break;
	case E_LAMBDA:
		fprintf(stream, "#<%p>", (void *)expr.box);
		break;
	case E_SPECIAL:
		fprintf(stream, "#<%s>", special_name(expr.special_type));
		break;
	}
}
