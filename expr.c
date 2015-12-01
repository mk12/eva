// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const struct { const char *name; int arity; } special_procs[N_SPECIAL_PROCS] = {
	{"eval", 1}, {"apply", 2},
	{"null?", 1}, {"symbol?", 1}, {"number?", 1}, {"boolean?", 1},
	{"pair?", 1}, {"procedure?", 1},
	{"eq?", 2},
	{"=", 2}, {"<", 2}, {">", 2}, {"<=", 2}, {">=", 2},
	{"cons", 2}, {"car", 1}, {"cdr", 1},
	{"+", -1}, {"-", -2}, {"*", -1}, {"/", -2}, {"remainder", 2},
	{"not", 1}
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

struct Expression new_number(int n) {
	return (struct Expression){ .type = E_NUMBER, .number = n };
}

struct Expression new_boolean(bool b) {
	return (struct Expression){ .type = E_BOOLEAN, .boolean = b };
}

struct Expression new_special(enum SpecialType type) {
	return (struct Expression){ .type = E_SPECIAL, .special_type = type };
}

struct Expression new_pair(struct Expression car, struct Expression cdr) {
	struct Box *box = malloc(sizeof *box);
	box->ref_count = 1;
	box->pair.car = car;
	box->pair.cdr = cdr;
	return (struct Expression){ .type = E_PAIR, .box = box };
}

struct Expression new_lambda(
		int arity, InternID *params, struct Expression body) {
	struct Box *box = malloc(sizeof *box);
	box->ref_count = 1;
	box->lambda.arity = arity;
	box->lambda.params = params;
	box->lambda.body = retain_expression(body);
	return (struct Expression){ .type = E_LAMBDA, .box = box };
}

static void dealloc_expression(struct Expression expr) {
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
	switch (expr.type) {
	case E_PAIR:
	case E_LAMBDA:
		expr.box->ref_count++;
		break;
	default:
		break;
	}
	return expr;
}

void release_expression(struct Expression expr) {
	switch (expr.type) {
	case E_PAIR:
	case E_LAMBDA:
		expr.box->ref_count--;
		if (expr.box->ref_count <= 0) {
			dealloc_expression(expr);
		}
		break;
	default:
		break;
	}
}

struct Expression clone_expression(struct Expression expr) {
	switch (expr.type) {
	case E_PAIR:
		return new_pair(
				clone_expression(expr.box->pair.car),
				clone_expression(expr.box->pair.cdr));
	case E_LAMBDA:;
		int n = expr.box->lambda.arity;
		InternID *params = malloc(n * sizeof *params);
		for (int i = 0; i < n; i++) {
			params[i] = expr.box->lambda.params[i];
		}
		return new_lambda(n, params, clone_expression(expr.box->lambda.body));
	default:
		return expr;
	}
}

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
		printf("%d", expr.number);
		break;
	case E_BOOLEAN:
		printf("#%c", expr.boolean ? 't' : 'f');
		break;
	case E_LAMBDA:
		printf("#<%p>", expr.box);
		break;
	case E_SPECIAL:
		printf("#<%s>", special_name(expr.special_type));
		break;
	}
}
