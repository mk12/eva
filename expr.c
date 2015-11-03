// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const SpecialProc special_procs[N_SPECIAL_PROCS] = {
	{"atom?", 1}, {"eq?", 2}, {"car", 1}, {"cdr", 1}, {"cons", 2},
	{"+", VAR_ARITY}, {"-", VAR_ARITY}, {"*", VAR_ARITY}, {"/", VAR_ARITY}
};

struct Expression *new_cons(struct Expression *car, struct Expression *cdr) {
	struct Expression *expr = malloc(sizeof *expr);
	expr->type = E_CONS;
	expr->cons.car = car;
	expr->cons.cdr = cdr;
	return expr;
}

struct Expression *new_null(void) {
	struct Expression *expr = malloc(sizeof *expr);
	expr->type = E_NULL;
	return expr;
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
	struct Expression *expr = malloc(sizeof *expr);
	expr->type = E_SPECIAL;
	expr->special.type = type;
	return expr;
}

struct Expression *clone_expression(struct Expression *expr) {
	switch (expr->type) {
	case E_NULL:
		return new_null();
	case E_CONS:
		return new_cons(
				clone_expression(expr->cons.car),
				clone_expression(expr->cons.cdr));
	case E_SYMBOL:
		return new_symbol(expr->symbol.name);
	case E_NUMBER:
		return new_number(expr->number.n);
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
	case E_CONS:
		free_expression(expr->cons.car);
		free_expression(expr->cons.cdr);
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

static void print_cons(struct Expression *expr, bool first) {
	if (!first) {
		putchar(' ');
	}
	print_expression(expr->cons.car);
	switch (expr->cons.cdr->type) {
	case E_NULL:
		putchar(')');
		break;
	case E_CONS:
		print_cons(expr->cons.cdr, false);
		break;
	default:
		fputs(" . ", stdout);
		print_expression(expr->cons.cdr);
		putchar(')');
		break;
	}
}

void print_expression(struct Expression *expr) {
	switch (expr->type) {
	case E_NULL:
		fputs("()", stdout);
		break;
	case E_CONS:
		putchar('(');
		print_cons(expr, true);
		break;
	case E_SYMBOL:
		fputs(expr->symbol.name, stdout);
		break;
	case E_NUMBER:
		printf("%d", expr->number.n);
		break;
	case E_LAMBDA:
		printf("#<%p>", expr->lambda.body);
		break;
	case E_SPECIAL:
		printf("#<%s>", special_procs[expr->special.type].name);
		break;
	}
}
