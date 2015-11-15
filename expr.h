// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

#include <stdbool.h>

// There are 7 types of expressions.
enum ExpressionType {
	E_NULL,
	E_SYMBOL,
	E_NUMBER,
	E_BOOLEAN,
	E_SPECIAL
	E_PAIR,
	E_LAMBDA,
};

#define N_SPECIAL_PROCS 20

// There are 20 types of special procedures. Special procedures are procedures
// implemented by the interpreter; they are distinct from special forms, which
// are not procedures becuase they require special evaluation rules.
enum SpecialType {
	S_NULL, S_PAIR, S_NUMBER, S_BOOLEAN, S_PROCEDURE,
	S_EQ, S_NUM_EQ, S_LT, S_GT, S_LE, S_GE,
	S_CONS, S_CAR, S_CDR,
	S_ADD, S_SUB, S_MUL, S_DIV, S_REM,
	S_NOT
};

// Expression is the algebraic data type used for all values in Scheme. Code and
// data are both represented as expressions. Five types of expressions fit in
// immediate values; the other two are stored in boxes.
struct Expression {
	enum ExpressionType type;
	union {
		int symbol_id;
		int number;
		bool boolean;
		enum SpecialType special;
		struct Box *box;
	};
};

// A box is a recursive structure that cannot be stored as an immediate value.
// It contains either a cons pair or a lambda expression. The type (pair or
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
			int *params;
			struct Expression body;
		} lambda;
	};
};

// Returns the arity or name of a special procedure.
int special_arity(enum SpecialType type);
const char *special_name(enum SpecialType type);

// Constructor functions for expressions.
struct Expression new_null(void);
struct Expression new_symbol(int id);
struct Expression new_number(int n);
struct Expression new_boolean(bool b);
struct Expression new_special(enum SpecialType type);
struct Expression new_pair(struct Expression car, struct Expression cdr);
struct Expression new_lambda(int arity, int *params, struct Expression body);

// Returns a deep copy of the expression.
struct Expression clone_expression(struct Expression expr);

// Decrements the reference count of all boxes in the expression.
void free_expression(struct Expression expr);

// Prints the expression to standard output (not followed by a newline).
void print_expression(struct Expression expr);

#endif
