// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef EXPR_H
#define EXPR_H

#include "intern.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct Environment;

// Types of expressions.
#define N_EXPRESSION_TYPES 13
enum ExpressionType {
	// Immediate expressions
	E_VOID,         // lack of a value
	E_NULL,         // empty list
	E_SYMBOL,       // interned string
	E_NUMBER,       // signed integer
	E_BOOLEAN,      // #t and #f
	E_CHARACTER,    // single character
	E_STDMACRO,     // standard macro (special form)
	E_STDPROCMACRO, // macro of standard procedure
	E_STDPROCEDURE, // standard procedure
	// Boxed expressions
	E_PAIR,         // cons cell
	E_STRING,       // string of text
	E_MACRO,        // user-defined macro
	E_PROCEDURE     // user-defined procedure
};

// Standard macros, also called special forms, are syntactical forms built into
// the language that require special evaluation rules.
#define N_STANDARD_MACROS 14
enum StandardMacro {
	// Definition and mutation
	F_DEFINE, F_SET,
	// Abstraction
	F_LAMBDA, F_BEGIN,
	// Quotation
	F_QUOTE, F_QUASIQUOTE, F_UNQUOTE, F_UNQUOTE_SPLICING,
	// Conditionals
	F_IF, F_COND,
	// Let bindings
	F_LET, F_LET_STAR,
	// Logical operators
	F_AND, F_OR
};

// Standard procedures are procedures implemented by the interpreter.
#define N_STANDARD_PROCEDURES 62
enum StandardProcedure {
	// Eval and apply
	S_EVAL, S_APPLY,
	// Macro creation
	S_MACRO,
	// Type predicates
	S_VOIDP, S_NULLP, S_SYMBOLP, S_NUMBERP, S_BOOLEANP, S_CHARP,
	S_PAIRP, S_STRINGP, S_MACROP, S_PROCEDUREP,
	// Equality (identity)
	S_EQ,
	// Numeric comparisons
	S_NUM_EQ, S_NUM_LT, S_NUM_GT, S_NUM_LE, S_NUM_GE,
	// Numeric operations
	S_ADD, S_SUB, S_MUL, S_DIV, S_REMAINDER, S_MODULO, S_EXPT,
	// Boolean negation
	S_NOT,
	// Character comparisons
	S_CHAR_EQ, S_CHAR_LT, S_CHAR_GT, S_CHAR_LE, S_CHAR_GE,
	// Pair constructor, accessors, and mutators
	S_CONS, S_CAR, S_CDR, S_SET_CAR, S_SET_CDR,
	// String functions
	S_MAKE_STRING, S_STRING_LENGTH, S_STRING_REF, S_STRING_SET,
	S_SUBSTRING, S_STRING_COPY, S_STRING_FILL, S_STRING_APPEND,
	// String comparisons
	S_STRING_EQ, S_STRING_LT, S_STRING_GT, S_STRING_LE, S_STRING_GE,
	// Conversion functions
	S_CHAR_TO_INTEGER, S_INTEGER_TO_CHAR,
	S_STRING_TO_SYMBOL, S_SYMBOL_TO_STRING,
	S_STRING_TO_NUMBER, S_NUMBER_TO_STRING,
	// Input/output
	S_READ, S_WRITE, S_DISPLAY, S_NEWLINE, S_ERROR, S_LOAD
};

// Number expressions are internally represented with long integers.
typedef long Number;

// Format string to use for Number in 'printf'.
extern const char *const NUMBER_FMT;

// Expression is the algebraic data type used for all values in Eva. Code and
// data are both represented as expressions. Seven types of expressions fit in
// immediate values; the other three are stored in boxes.
struct Expression {
	enum ExpressionType type;
	union {
		InternId symbol_id;
		Number number;
		bool boolean;
		char character;
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
		// Used by E_STRING:
		struct {
			char* str;
			size_t len;
		};
		// Used by E_MACRO and E_PROCEDURE:
		struct {
			Arity arity;
			struct Expression *params;
			struct Expression body;
			struct Environment *env;
		};
	};
};

// Returns the user-facing name of the expression type in uppercase letters.
const char *expression_type_name(enum ExpressionType type);

// Returns a base environment containing mappings for all standard macros, all
// standard procedures, and the symbol "else".
struct Environment *new_standard_environment(void);

// Constructors for immediate expressions.
struct Expression new_void(void);
struct Expression new_null(void);
struct Expression new_symbol(InternId symbol_id);
struct Expression new_number(Number number);
struct Expression new_boolean(bool boolean);
struct Expression new_character(char character);
struct Expression new_stdmacro(enum StandardMacro stdmacro);
struct Expression new_stdprocedure(enum StandardProcedure stdproc);

// Creates a new pair. Sets the reference count of the box to 1. Takes owernship
// of 'car' and 'cdr' without retaining them.
struct Expression new_pair(struct Expression car, struct Expression cdr);

// Creates a new string. Sets the reference count of the box to 1. Takes
// ownership of the string buffer and frees it on deallocation.
struct Expression new_string(char *str, size_t len);

// Creates a new macro based on an expression of type E_STDPROCEDURE (resulting
// in E_STDPROCMACRO) or E_PROCEDURE (resulting in E_MACRO). Takes ownership of
// 'expr' without retaining it.
struct Expression new_macro(struct Expression expr);

// Creates a new procedure. Sets the reference count of the box to 1. Takes
// ownership of 'params' without copying the array. Takes ownership of 'body'
// and 'env' without retaining them.
struct Expression new_procedure(
		Arity arity,
		struct Expression *params,
		struct Expression body,
		struct Environment *env);

// Increments the reference count of the expression's box. This is a no-op for
// immediates. Returns the expression for convenience.
struct Expression retain_expression(struct Expression expr);

// Decrements the reference count of the expression's box. This is a no-op for
// immediates. Deallocates the expression if the reference count reaches zero.
void release_expression(struct Expression expr);

// Returns true if the expression is "truthy" (anything except #f).
bool expression_truthy(struct Expression expr);

// Returns true if expressions 'lhs' and 'rhs' are identical in the sense of the
// Scheme predicate "eq?". Immediate expressions (of the same type) are
// identical if they represent the same value. Boxed expressions (of the same
// type) are identical if they point to the same box in memory.
bool expression_eq(struct Expression lhs, struct Expression rhs);

// Returns true if the expression is callable, and stores its arity in 'out'.
// Returns false otherwise. Expressions of types E_STDMACRO, E_STDPROCEDURE,
// E_MACRO, and E_PROCEDURE are callable.
bool expression_arity(Arity *out, struct Expression expr);

// Returns true if the given arity accepts 'n_args' arguments.
bool arity_allows(Arity arity, size_t n_args);

// Creates a C-style null-terminated string from string expression. Assumes that
// 'expr' has type E_STRING. The caller is responsible for freeing the result.
char* null_terminated_string(struct Expression expr);

// Prints the expression to 'stream' (not followed by a newline).
void print_expression(struct Expression expr, FILE *stream);

#endif
