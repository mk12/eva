// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

#include "intern.h"

#include <stdbool.h>

// There are 7 types of expressions.
enum ExpressionType {
	// Immediate expressions:
	E_NULL,
	E_SYMBOL,
	E_NUMBER,
	E_BOOLEAN,
	E_SPECIAL,
	// Boxed expressions:
	E_PAIR,
	E_LAMBDA
};

#define N_SPECIAL_PROCS 23

// There are 20 special procedures. Special procedures (as distinct from special
// *forms*, which require special evaluation rules) are implemented by the
// interpreter.
enum SpecialType {
	// Eval and apply:
	S_EVAL, S_APPLY,
	// Type predicates:
	S_NULL, S_SYMBOL, S_NUMBER, S_BOOLEAN, S_PAIR, S_PROCEDURE,
	// Equality (identity):
	S_EQ,
	// Numeric comparisons:
	S_NUM_EQ, S_LT, S_GT, S_LE, S_GE,
	// Pair constructor and accessors:
	S_CONS, S_CAR, S_CDR,
	// Numeric operations:
	S_ADD, S_SUB, S_MUL, S_DIV, S_REM,
	// Boolean negation (and/or are special forms):
	S_NOT
};

// Expression is the algebraic data type used for all values in Scheme. Code and
// data are both represented as expressions. Five types of expressions fit in
// immediate values; the other two are stored in boxes.
struct Expression {
	enum ExpressionType type;
	union {
		InternID symbol_id;
		int number;
		bool boolean;
		enum SpecialType special_type;
		struct Box *box;
	};
};

// A box is a recursive structure that cannot be stored as an immediate value.
// It contains either a cons pair or a lambda expression. The type tag (pair or
// lambda) is stored in the expression that points to the box.
struct Box {
	int ref_count;
	union {
		struct {
			struct Expression car;
			struct Expression cdr;
		} pair;
		struct {
			int arity;
			InternID *params;
			struct Expression body;
		} lambda;
	};
};

// Returns the arity or name of a special procedure.
int special_arity(enum SpecialType type);
const char *special_name(enum SpecialType type);

// Constructor functions for immediate expressions.
struct Expression new_null(void);
struct Expression new_symbol(InternID id);
struct Expression new_number(int n);
struct Expression new_boolean(bool b);
struct Expression new_special(enum SpecialType type);

// Constructor functions for boxed expressions. Sets the reference count to 1.
struct Expression new_pair(struct Expression car, struct Expression cdr);
struct Expression new_lambda(
		int arity, InternID *params, struct Expression body);

// Increments the reference count of the box. This is a no-op for immediates.
// Returns the expression for convenience.
struct Expression retain_expression(struct Expression expr);

// Decrements the reference count of the box. This is a no-op for immediates.
// Also deallocates the expression if the reference count reaches zero.
void release_expression(struct Expression expr);

// Returns a deep copy of the expression by recursively created new boxes. Does
// not alter the reference counts of the source expression.
struct Expression clone_expression(struct Expression expr);

// Prints the expression to standard output (not followed by a newline).
void print_expression(struct Expression expr);

#endif
