// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef ERROR_H
#define ERROR_H

#include "expr.h"
#include "intern.h"

#include <stddef.h>

// Error types for parse errors.
enum ParseErrorType {
	ERR_EXPECTED_RPAREN,
	ERR_INVALID_DOT,
	ERR_INVALID_LITERAL,
	ERR_UNEXPECTED_EOI,
	ERR_UNEXPECTED_RPAREN
};

// Error types for evaluation errors.
enum EvalErrorType {
	                    // Fields of EvalErorr used:
	ERR_ARITY,          // arity, n_args
	ERR_DIV_ZERO,       // (none)
	ERR_DUP_PARAM,      // symbol_id
	ERR_INVALID_VAR,    // symbol_id
	ERR_NON_EXHAUSTIVE, // (none)
	ERR_OP_NOT_PROC,    // expr
	ERR_PROC_CALL,      // (none)
	ERR_SYNTAX,         // str
	ERR_TYPE,           // expected, arg_pos, expr
	ERR_UNBOUND_VAR     // symbol_id
};

// An error that occurs during file manipulation.
struct FileError {
	int errno;             // errno value set by the IO function
	const char *filename;  // name of the file being manipulated
};

// An error that causes the parse to fail.
struct ParseError {
	enum ParseErrorType type;
	const char *filename; // name of input source
	const char *text;     // full text being parsed
	size_t index;         // index in 'text' where the error occurred
};

// A runtime error that occurs during code evaluation.
struct EvalError {
	enum EvalErrorType type;
	union {
		const char *str;    // a string relevant to the error
		InternId symbol_id; // a symbol relevant to the error
		struct {
			enum ExpressionType expected; // expected type
			size_t arg_pos;               // argument position (zero-based)
			struct Expression expr;       // provided expression
		};
		struct {
			int arity;     // sign-encoded arity (see Expr.h)
			size_t n_args; // number of arguments provided
		};
	};
};

// Constructors for evaluation errors. These allocate memory.
struct EvalError *new_eval_error(enum EvalErrorType type);
struct EvalError *new_eval_error_symbol(enum EvalErrorType type, InternId id);
struct EvalError *new_syntax_error(const char *str);
struct EvalError *new_type_error(
	enum ExpressionType expected, size_t arg_pos, struct Expression expr);

// Prints a generic error message to standard error.
void print_error(const char *err_msg);

// Prints a file error to standard error.
void print_file_error(const struct FileError *err);

// Prints a parse error to standard error. Prints the file name, line number,
// column, error message, and the problematic line of code.
void print_parse_error(const struct ParseError *err);

// Prints an evaluation error to standard error. Prints the error message and,
// for certain error types, other information relevant to the error.
void print_eval_error(const struct EvalError *err);

#endif
