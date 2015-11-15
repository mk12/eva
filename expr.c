// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const struct SpecialProc special_procs[N_SPECIAL_PROCS] = {
	{"null?", 1}, {"pair?", 1}, {"number?", 1}, {"boolean?", 1},
	{"procedure?", 1},
	{"eq?", 2}, {"=", 2}, {"<", 2}, {">", 2}, {"<=", 2}, {">=", 2},
	{"cons", 2}, {"car", 1}, {"cdr", 1},
	{"+", -1}, {"-", -2}, {"*", -1}, {"/", -2}, {"remainder", 2},
	{"not", 1}
};

static const struct Expression expr_null = { .type = E_NULL };

static const struct Expression expr_boolean[2] = {
	{ .type = E_BOOLEAN, .boolean = { false } },
	{ .type = E_BOOLEAN, .boolean = { true } }
};

static const struct Expression expr_special[N_SPECIAL_PROCS] = {
	{ .type = E_SPECIAL, .special = { S_NULL } },
	{ .type = E_SPECIAL, .special = { S_PAIR } },
	{ .type = E_SPECIAL, .special = { S_NUMBER } },
	{ .type = E_SPECIAL, .special = { S_BOOLEAN } },
	{ .type = E_SPECIAL, .special = { S_PROCEDURE } },
	{ .type = E_SPECIAL, .special = { S_EQ } },
	{ .type = E_SPECIAL, .special = { S_NUM_EQ } },
	{ .type = E_SPECIAL, .special = { S_LT } },
	{ .type = E_SPECIAL, .special = { S_GT } },
	{ .type = E_SPECIAL, .special = { S_LE } },
	{ .type = E_SPECIAL, .special = { S_GE } },
	{ .type = E_SPECIAL, .special = { S_CONS } },
	{ .type = E_SPECIAL, .special = { S_CAR } },
	{ .type = E_SPECIAL, .special = { S_CDR } },
	{ .type = E_SPECIAL, .special = { S_ADD } },
	{ .type = E_SPECIAL, .special = { S_SUB } },
	{ .type = E_SPECIAL, .special = { S_MUL } },
	{ .type = E_SPECIAL, .special = { S_DIV } },
	{ .type = E_SPECIAL, .special = { S_REM } },
	{ .type = E_SPECIAL, .special = { S_NOT } },
};

// global symbol table

struct Expression *new_pair(struct Expression *car, struct Expression *cdr) {
	struct Expression *expr = malloc(sizeof *expr);
	expr->type = E_PAIR;
	expr->pair.car = car;
	expr->pair.cdr = cdr;
	return expr;
}

struct Expression *new_null(void) {
	return &expr_null;
}

struct Expression *new_symbol(char *name) {
	struct Expression *expr = malloc(sizeof *expr);
	expr->type = E_SYMBOL;
	expr->symbol.name = name;
	return expr;
}

struct Expression *new_number(int n) {
	struct Expression *expr = malloc(sizeof *expr);
	expr->type = E_NUMBER;
	expr->number.n = n;
	return expr;
}

struct Expression *new_boolean(bool b) {
	return expr_boolean + b;
}

struct Expression *new_lambda(
		int arity, char **params, struct Expression *body) {
	struct Expression *expr = malloc(sizeof *expr);
	expr->type = E_LAMBDA;
	expr->lambda.arity = arity;
	expr->lambda.params = params;
	expr->lambda.body = body;
	return expr;
}

struct Expression *new_special(enum SpecialType type) {
	return expr_special + type;
}

struct Expression *clone_expression(struct Expression *expr) {
	switch (expr->type) {
	case E_NULL:
		return new_null();
	case E_PAIR:
		return new_pair(
				clone_expression(expr->pair.car),
				clone_expression(expr->pair.cdr));
	case E_SYMBOL:
		return new_symbol(expr->symbol.name);
	case E_NUMBER:
		return new_number(expr->number.n);
	case E_BOOLEAN:
		return new_boolean(expr->boolean.b);
	case E_LAMBDA:;
		int n = expr->lambda.arity;
		char **params = malloc(n * sizeof *params);
		for (int i = 0; i < n; i++) {
			params[i] = strdup(expr->lambda.params[i]);
		}
		return new_lambda(n, params, clone_expression(expr->lambda.body));
	case E_SPECIAL:
		return new_special(expr->special.type);
	}
}

void free_expression(struct Expression *expr) {
	switch (expr->type) {
	case E_PAIR:
		free_expression(expr->pair.car);
		free_expression(expr->pair.cdr);
		break;
	case E_SYMBOL:
		free(expr->symbol.name);
		break;
	case E_LAMBDA:
		for (int i = 0; i < expr->lambda.arity; i++) {
			free(expr->lambda.params[i]);
		}
		free(expr->lambda.params);
		free_expression(expr->lambda.body);
	default:
		break;
	}
	free(expr);
}

static void print_pair(struct Expression *expr, bool first) {
	if (!first) {
		putchar(' ');
	}
	print_expression(expr->pair.car);
	switch (expr->pair.cdr->type) {
	case E_NULL:
		putchar(')');
		break;
	case E_PAIR:
		print_pair(expr->pair.cdr, false);
		break;
	default:
		fputs(" . ", stdout);
		print_expression(expr->pair.cdr);
		putchar(')');
		break;
	}
}

void print_expression(struct Expression *expr) {
	switch (expr->type) {
	case E_NULL:
		fputs("()", stdout);
		break;
	case E_PAIR:
		putchar('(');
		print_pair(expr, true);
		break;
	case E_SYMBOL:
		fputs(expr->symbol.name, stdout);
		break;
	case E_NUMBER:
		printf("%d", expr->number.n);
		break;
	case E_BOOLEAN:
		printf("#%c", expr->boolean.b ? 't' : 'f');
		break;
	case E_LAMBDA:
		printf("#<%p>", expr->lambda.body);
		break;
	case E_SPECIAL:
		printf("#<%s>", special_procs[expr->special.type].name);
		break;
	}
}
