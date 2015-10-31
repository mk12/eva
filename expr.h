// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

struct Variable {
	char *name;
};

enum ExpressionType {
	E_NULL,
	E_CONS,
	E_SYMBOL,
	E_NUMBER,
	E_LAMBDA,
	E_SPECIAL
};

enum SpecialType {
	S_ATOM,
	S_EQ,
	S_CAR,
	S_CDR,
	S_CONS,
	S_ADD,
	S_SUB,
	S_MUL,
	S_DIV
};

struct Expression {
	enum ExpressionType type;
	union {
		struct {
			int n;
		} number;
		struct {
			char *name;
		} symbol;
		struct {
			struct Expression *car;
			struct Expression *cdr;
		} cons;
		struct {
			int arity;
			struct Variable *params;
			struct Expression *body;
		} lambda;
		struct {
			enum SpecialType type;
		} special;
	};
};

struct Expression *new_cons(struct Expression *car, struct Expression *cdr);
struct Expression *new_null(void);
struct Expression *new_symbol(char *name);
struct Expression *new_number(int n);
struct Expression *new_lambda(
		int arity, struct Variable *params, struct Expression *body);
struct Expression *new_special(enum SpecialType type);

void print_expression(struct Expression *expr);
void free_expression(struct Expression *expr);

#endif
