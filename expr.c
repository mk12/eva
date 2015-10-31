// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include <stdlib.h>

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
		int arity, struct Variable *params, struct Expression *body) {
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

void print_expression(struct Expression *expr) {
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
		free(expr->lambda.params);
		free_expression(expr->lambda.body);
	default:
		break;
	}
	free(expr);
}
