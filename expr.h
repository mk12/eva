// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

#include <stdbool.h>

enum ExpressionType {
	E_NULL,
	E_PAIR,
	E_SYMBOL,
	E_NUMBER,
	E_BOOLEAN,
	E_LAMBDA,
	E_SPECIAL
};

enum SpecialType {
	S_NULL, S_PAIR, S_NUMBER, S_BOOLEAN, S_PROCEDURE,
	S_EQ, S_NUM_EQ, S_LT, S_GT, S_LE, S_GE,
	S_CONS, S_CAR, S_CDR,
	S_ADD, S_SUB, S_MUL, S_DIV, S_REM,
	S_NOT
};

// A SpecialProc is a procedure implemented by the interpreter. Each expression
// of type E_SPECIAL represents on the the special procedures. Note that special
// procedures are distinct from special forms: the latter do not follow the
// usual evaluation rules.
struct SpecialProc {
	const char *name;
	int arity;
};

#define N_SPECIAL_PROCS 20

// The special_procs array is indexed by SpecialType values.
extern const struct SpecialProc special_procs[N_SPECIAL_PROCS];

// Expression is an algebraic data type used for all terms in Scheme. Code and
// data are both represented as expressions.
struct Expression {
	enum ExpressionType type;
	int ref_count; // TODO: use this
	union {
		struct {
			struct Expression *car;
			struct Expression *cdr;
		} pair;
		struct {
			char *name;
		} symbol;
		struct {
			int n;
		} number;
		struct {
			bool b;
		} boolean;
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
struct Expression *new_pair(struct Expression *car, struct Expression *cdr);
struct Expression *new_null(void);
struct Expression *new_symbol(char *name);
struct Expression *new_number(int n);
struct Expression *new_boolean(bool b);
struct Expression *new_lambda(
		int arity, char **params, struct Expression *body);
struct Expression *new_special(enum SpecialType type);

// Returns a deep copy of the expression.
struct Expression *clone_expression(struct Expression *expr);

// Frees the expression and all its subexpressions.
void free_expression(struct Expression *expr);

// Prints the expression to standard output (not followed by a newline).
void print_expression(struct Expression *expr);

#endif
