// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REF_COUNT_LOGGING 0

// Counters for the total number of allocated boxes and the total of all
// reference counts, used for debugging.
#if REF_COUNT_LOGGING
static int total_box_count = 0;
static int total_ref_count = 0;
#endif

// Names and arities of special procedures.
static const struct {
	const char *name;
	int arity;
} special_procs[N_SPECIAL_PROCS] = {
	{"eval", 1}, {"apply", 2},
	{"null?", 1}, {"symbol?", 1}, {"number?", 1}, {"boolean?", 1},
	{"pair?", 1}, {"procedure?", 1},
	{"eq?", 2},
	{"=", 2}, {"<", 2}, {">", 2}, {"<=", 2}, {">=", 2},
	{"cons", 2}, {"car", 1}, {"cdr", 1},
	{"+", -1}, {"-", -2}, {"*", -1}, {"/", -2}, {"remainder", 2},
	{"not", 1},
	{"read", 0}, {"write", 1}
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

struct Expression new_symbol(InternID id) {
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

// Log information about reference counts.
#if REF_COUNT_LOGGING
static void log_ref_count(const char *action, struct Expression expr) {
	bool pair = expr.type == E_PAIR;
	assert(pair || expr.type == E_LAMBDA);
	printf("[%d/%d] %7s %6s [%d] ", total_ref_count, total_box_count, action,
			pair ? "pair" : "lambda", expr.box->ref_count);
	if (pair) {
		putchar('(');
		print_expression(expr.box->pair.car);
		fputs(" . ", stdout);
		print_expression(expr.box->pair.cdr);
	} else {
		fputs("(lambda (?) ", stdout);
		print_expression(expr.box->lambda.body);
	}
	fputs(")\n", stdout);
}
#endif

struct Expression new_pair(struct Expression car, struct Expression cdr) {
	struct Box *box = malloc(sizeof *box);
	box->ref_count = 1;
	box->pair.car = car;
	box->pair.cdr = cdr;
	struct Expression expr = { .type = E_PAIR, .box = box };
#if REF_COUNT_LOGGING
	total_box_count++;
	total_ref_count++;
	log_ref_count("create", expr);
#endif
	return expr;
}

struct Expression new_lambda(
		int arity, InternID *params, struct Expression body) {
	struct Box *box = malloc(sizeof *box);
	box->ref_count = 1;
	box->lambda.arity = arity;
	box->lambda.params = params;
	box->lambda.body = body;
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
		release_expression(expr.box->pair.car);
		release_expression(expr.box->pair.cdr);
		free(expr.box);
		break;
	case E_LAMBDA:
		free(expr.box->lambda.params);
		release_expression(expr.box->lambda.body);
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

// Prints a pair to standard output, assuming the left parenthesis has already
// been printed. Uses standard Lisp s-expression notation.
static void print_pair(struct Box *box, bool first) {
	if (!first) {
		putchar(' ');
	}
	print_expression(box->pair.car);
	switch (box->pair.cdr.type) {
	case E_NULL:
		putchar(')');
		break;
	case E_PAIR:
		print_pair(box->pair.cdr.box, false);
		break;
	default:
		// Print a dot before the last cdr if it is not null.
		fputs(" . ", stdout);
		print_expression(box->pair.cdr);
		putchar(')');
		break;
	}
}

void print_expression(struct Expression expr) {
	switch (expr.type) {
	case E_NULL:
		fputs("()", stdout);
		break;
	case E_PAIR:
		putchar('(');
		print_pair(expr.box, true);
		break;
	case E_SYMBOL:
		fputs(find_string(expr.symbol_id), stdout);
		break;
	case E_NUMBER:
		printf("%ld", expr.number);
		break;
	case E_BOOLEAN:
		printf("#%c", expr.boolean ? 't' : 'f');
		break;
	case E_LAMBDA:
		printf("#<%p>", (void *)expr.box);
		break;
	case E_SPECIAL:
		printf("#<%s>", special_name(expr.special_type));
		break;
	}
}
