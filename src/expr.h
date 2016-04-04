// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

#include "intern.h"

#include <stdbool.h>
#include <stdio.h>

// Types of expressions.
#define N_EXPRESSION_TYPES 7
enum ExpressionType {
	// Immediate expressions:
	E_NULL,
	E_SYMBOL,
	E_NUMBER,
	E_BOOLEAN,
	E_STDPROC,
	// Boxed expressions:
	E_PAIR,
	E_LAMBDA
};

// Standard procedures are procedures implemented by the interpreter.
#define N_STANDARD_PROCS 25
enum StandardProc {
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
	// Boolean negation (and/or are special forms for short-circuiting):
	S_NOT,
	// Reading and writing:
	S_READ, S_WRITE
};

// Expression is the algebraic data type used for all values in Eva. Code and
// data are both represented as expressions. Five types of expressions fit in
// immediate values; the other two are stored in boxes.
struct Expression {
	enum ExpressionType type;
	union {
		InternId symbol_id;
		long number;
		bool boolean;
		enum StandardProc stdproc;
		struct Box *box;
	};
};

// A box is a recursive structure that cannot be stored as an immediate value.
// It contains either a cons pair or a lambda expression. The type tag (pair or
// lambda) is stored in the expression pointing to the box, not in the box.
struct Box {
	int ref_count;
	union {
		struct {
			struct Expression car;
			struct Expression cdr;
		};
		struct {
			// Procedures use sign-encoded arity. If the arity is N >= 0, the
			// procedure requires exactly N arguments. If N < 0, it accepts
			// -(N+1) or more arguments, but not less. In that case, the last
			// element of 'params' will be bound to a list containing arguments
			// in positions -(N+1)+1, -(N+1)+2, and so on.
			int arity;
			InternId *params;
			struct Expression body;
		};
	};
};

// Arity and name accessors for standard procedures.
int stdproc_arity(enum StandardProc stdproc);
const char *stdproc_name(enum StandardProc stdproc);

// Constructors for immediate expressions.
struct Expression new_null(void);
struct Expression new_symbol(InternId symbol_id);
struct Expression new_number(long number);
struct Expression new_boolean(bool boolean);
struct Expression new_stdproc(enum StandardProc stdproc);

// Constructors for boxed expressions. They set the reference count to 1 and
// treat subexpression arguments ('car', 'cdr', and 'body') as being moved into
// the new object; that is, they take ownership without retaining them.
struct Expression new_pair(struct Expression car, struct Expression cdr);
struct Expression new_lambda(
		int arity, InternId *params, struct Expression body);

// Increments the reference count of the box. This is a no-op for immediates.
// Returns the expression for convenience.
struct Expression retain_expression(struct Expression expr);

// Decrements the reference count of the box. This is a no-op for immediates.
// Also deallocates the expression if the reference count reaches zero.
void release_expression(struct Expression expr);

// Returns true if expressions 'lhs' and 'rhs' are identical, in the sense of
// the Scheme predicate "eq?". Immediate expressions (of the same type) are
// identical if they represent the same value. Boxed expressions (of the same
// type) are identical if they point to the same box in memory.
bool expression_eq(struct Expression lhs, struct Expression rhs);

// Prints the expression to 'stream' (not followed by a newline).
void print_expression(struct Expression expr, FILE *stream);

#endif
