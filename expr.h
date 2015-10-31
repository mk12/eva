// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

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

// Expression is an algebraic data type used for all terms in Scheme. Code and
// data are both represented as expressions.
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
			char **params;
			struct Expression *body;
		} lambda;
		struct {
			enum SpecialType type;
		} special;
	};
};

// Constructor functions for expressions.
struct Expression *new_cons(struct Expression *car, struct Expression *cdr);
struct Expression *new_null(void);
struct Expression *new_symbol(char *name);
struct Expression *new_number(int n);
struct Expression *new_lambda(
		int arity, char **params, struct Expression *body);
struct Expression *new_special(enum SpecialType type);

// Prints the expression to standard output (not followed by a newline).
void print_expression(struct Expression *expr);

// Frees the expression and all its subexpressions.
void free_expression(struct Expression *expr);

#endif
