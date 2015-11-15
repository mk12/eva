// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const struct { const char *name, int arity } special_procs[N_SPECIAL_PROCS] = {
	{"null?", 1}, {"pair?", 1}, {"number?", 1}, {"boolean?", 1},
	{"procedure?", 1},
	{"eq?", 2}, {"=", 2}, {"<", 2}, {">", 2}, {"<=", 2}, {">=", 2},
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
	return { .type = E_NULL };
}

struct Expression new_symbol(char *name) {
	return { .type = E_SYMBOL, .symbol = name };
}

struct Expression new_number(int n) {
	return { .type = E_NUMBER, .number = n };
}

struct Expression new_boolean(bool b) {
	return { .type = E_BOOL, .boolean = b };
}

struct Expression new_special(enum SpecialType type) {
	return { .type = E_SPECIAL, .special = type };
}

struct Expression new_pair(struct Expression car, struct Expression cdr) {
	struct Box *box = malloc(sizeof *box);
	box->pair.car = car;
	box->pair.cdr = cdr;
	return { .type = E_PAIR, .box = box };
}

struct Expression new_lambda(int arity, char **params, struct Expression body) {
	struct Expression *box = malloc(sizeof *box);
	box->lambda.arity = arity;
	box->lambda.params = params;
	box->lambda.body = body;
	return { .type = E_LAMBDA, .box = box };
}

struct Expression clone_expression(struct Expression expr) {
	switch (expr.type) {
	case E_PAIR:
		return new_pair(
				clone_expression(expr->box.pair.car),
				clone_expression(expr->box.pair.cdr));
	case E_LAMBDA:;
		int n = expr->box.lambda.arity;
		char **params = malloc(n * sizeof *params);
		for (int i = 0; i < n; i++) {
			params[i] = strdup(expr->lambda.params[i]);
		}
		return new_lambda(n, params, clone_expression(expr->lambda.body));
	default:
		return expr;
	}
}

void free_expression(struct Expression expr) {
	switch (expr.type) {
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

static void print_pair(struct Box *box, bool first) {
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
		fputs(expr->symbol.name, stdout);
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
		printf("#<%s>", special_name(expr.special));
		break;
	}
}
