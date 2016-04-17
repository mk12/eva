// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

#include "intern.h"

#include <stdbool.h>
#include <stdio.h>

struct Environment;

// Types of expressions.
#define N_EXPRESSION_TYPES 9
enum ExpressionType {
	// Immediate expressions
	E_NULL,         // empty list
	E_SYMBOL,       // interned string
	E_NUMBER,       // 64-bit signed integer
	E_BOOLEAN,      // #t and #f
	E_STDMACRO,     // standard macro (special form)
	E_STDPROCEDURE, // standard procedure
	// Boxed expressions
	E_PAIR,         // cons cell
	E_MACRO,        // user-defined macro
	E_PROCEDURE     // user-defined procedure
};

// Standard macros, also called special forms, are syntactical forms built into
// the language that require special evaluation rules.
#define N_STANDARD_MACROS 15
enum StandardMacro {
	// Definitions
	F_DEFINE, F_SET_BANG,
	// Abstraction
	F_LAMBDA, F_BEGIN,
	// Quotation
	F_QUOTE, F_QUASIQUOTE, F_UNQUOTE, F_UNQUOTE_SPLICING,
	// Conditionals
	F_IF, F_COND,
	// Let bindings
	F_LET, F_LET_STAR, F_LET_REC,
	// Logical operators
	F_AND, F_OR
};

// Standard procedures are procedures implemented by the interpreter.
#define N_STANDARD_PROCEDURES 27
enum StandardProcedure {
	// Eval and apply
	S_EVAL, S_APPLY,
	// Macro creation
	S_MACRO,
	// Type predicates
	S_NULLP, S_SYMBOLP, S_NUMBERP, S_BOOLEANP, S_PAIRP, S_MACROP, S_PROCEDUREP,
	// Equality (identity)
	S_EQ,
	// Numeric comparisons
	S_NUM_EQ, S_NUM_LT, S_NUM_GT, S_NUM_LE, S_NUM_GE,
	// Pair constructor and accessors
	S_CONS, S_CAR, S_CDR,
	// Numeric operations
	S_ADD, S_SUB, S_MUL, S_DIV, S_REM,
	// Boolean negation
	S_NOT,
	// Reading and writing
	S_READ, S_WRITE
};

// Expression is the algebraic data type used for all values in Eva. Code and
// data are both represented as expressions. Six types of expressions fit in
// immediate values; the other three are stored in boxes.
struct Expression {
	enum ExpressionType type;
	union {
		InternId symbol_id;
		long number;
		bool boolean;
		enum StandardMacro stdmacro;
		enum StandardProcedure stdproc;
		struct Box *box;
	};
};

// The arity of a macro or procedure is the number of arguments it takes.
// Arity is represented by a signed integer N. If N >= 0, the macro or procedure
// requires exactly N arguments. If N < 0, it accepts -(N+1) or more arguments,
// but not less. In that case, the last formal parameter will be bound to a list
// containing arguments in positions -(N+1)+1, -(N+1)+2, and so on.
typedef int Arity;

// ATLEAST(n) produces a sign-encoded arity specifying 'n' or more arguments.
// It also performs the inverse: ATLEAST(ATLEAST(n)) == n.
#define ATLEAST(n) (-((n)+1))

// A box is a recursive structure that cannot be stored as an immediate value.
// It contains a cons pair, macro, or procedure. The type tag is stored in the
// expression pointing to the box, not in the box itself. Box memory is managed
// by reference counting (see 'retain_expression' and 'release_expression').
struct Box {
	int ref_count;
	union {
		// Used by E_PAIR:
		struct {
			struct Expression car;
			struct Expression cdr;
		};
		// Used by E_MACRO and E_PROCEDURE:
		struct {
			Arity arity;
			InternId *params;
			struct Expression body;
			struct Environment *env;
		};
	};
};

// Returns the name of the expression type in uppercase letters. This does not
// distinguish standard and non-standard types, so E_STDMACRO and E_STDPROCEDURE
// will return the same names as E_MACRO and E_PROCDURE, respectively.
const char *expression_type_name(enum ExpressionType type);

// Returns a base environment containing mappings for all standard macros, all
// standard procedures, and the symbol "else".
struct Environment *new_standard_environment(void);

// Constructors for immediate expressions.
struct Expression new_null(void);
struct Expression new_symbol(InternId symbol_id);
struct Expression new_number(long number);
struct Expression new_boolean(bool boolean);
struct Expression new_stdmacro(enum StandardMacro stdmacro);
struct Expression new_stdprocedure(enum StandardProcedure stdproc);

// Constructors for boxed expressions. They set the reference count to 1 and
// treat subexpression arguments ('car', 'cdr', and 'body') as being moved into
// the new object; that is, they take ownership without retaining them. The
// environment 'env' is retained by 'new_procedure'.
struct Expression new_pair(struct Expression car, struct Expression cdr);
struct Expression new_procedure(
		Arity arity,
		InternId *params,
		struct Expression body,
		struct Environment *env);

// Increments the reference count of the box. This is a no-op for immediates.
// Returns the expression for convenience.
struct Expression retain_expression(struct Expression expr);

// Decrements the reference count of the box. This is a no-op for immediates.
// Deallocates the expression if the reference count reaches zero.
void release_expression(struct Expression expr);

// Returns true if expressions 'lhs' and 'rhs' are identical in the sense of the
// Scheme predicate "eq?". Immediate expressions (of the same type) are
// identical if they represent the same value. Boxed expressions (of the same
// type) are identical if they point to the same box in memory.
bool expression_eq(struct Expression lhs, struct Expression rhs);

// Returns true if the expression is callable. Expressions of types E_STDMACRO,
// E_STDPROCEDURE, E_MACRO, and E_PROCEDURE are callable.
bool expression_callable(struct Expression expr);

// Returns true if the expression is callable and accepts 'n' arguments.
bool accepts_n_arguments(struct Expression expr, size_t n);

// Prints the expression to 'stream' (not followed by a newline).
void print_expression(struct Expression expr, FILE *stream);

#endif
