// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef ERROR_H
#define ERROR_H

#include "expr.h"
#include "intern.h"

#include <stddef.h>

// Filename to use when input is from standard input.
extern const char *const stdin_filename;

// Error types for parse errors.
#define N_PARSE_ERROR_TYPES 5
enum ParseErrorType {
	ERR_EXPECTED_RPAREN,
	ERR_INVALID_DOT,
	ERR_INVALID_LITERAL,
	ERR_UNEXPECTED_EOI,
	ERR_UNEXPECTED_RPAREN
};

// Error types for evaluation errors.
#define N_EVAL_ERROR_TYPES 11
enum EvalErrorType {
	                    // Fields of EvalErorr used:
	ERR_ARITY,          // code, arity, n_args
	ERR_DEFINE,         // code
	ERR_DIV_ZERO,       // code
	ERR_DUP_PARAM,      // code, intern_id
	ERR_NON_EXHAUSTIVE, // code
	ERR_READ,           // parse_err
	ERR_SYNTAX,         // code
	ERR_TYPE_OPERAND,   // code, expected_type, position, expr
	ERR_TYPE_OPERATOR,  // code, expr
	ERR_TYPE_OTHER,     // code, expected_type, expr
	ERR_UNBOUND_VAR     // code, intern_id
};

// An error that causes the parse to fail.
struct ParseError {
	enum ParseErrorType type;
	char *text;     // full text being parsed
	size_t index;   // index in 'text' where the error occurred
	bool owns_text; // whether the error owns the memory of 'text'
};

// A runtime error that occurs during code evaluation.
struct EvalError {
	enum EvalErrorType type;
	struct Expression code; // context of the error
	union {
		// Used by ERR_DUP_PARAM and ERR_UNBOUND_VAR:
		InternId intern_id;
		// Used by ERR_READ:
		struct ParseError *parse_err;
		// Used by ERR_ARITY:
		struct {
			Arity arity;
			size_t n_args;
		};
		// Used by ERR_TYPE_*:
		struct {
			enum ExpressionType expected_type;
			int arg_pos;
			struct Expression expr;
		};
	};
};

// Constructors for parse errors and evaluation errors.
struct ParseError *new_parse_error(
		enum ParseError type, char *text, size_t index, bool owns_text);
struct EvalError *new_eval_error(enum EvalErrorType type);
struct EvalError *new_type_error(
		enum ExpressionType expected_type,
		const struct Expression *args,
		size_t arg_pos);

// Destructors for parse errors and evaluation errors.
void free_parse_error(struct ParseError *err);
void free_eval_error(struct EvalError *err);

// Prints a generic error message to standard error.
void print_error(const char *err_msg);

// Prints a file error to standard error based on the value of global 'errno'.
void print_file_error(const char *filename);

// Prints a parse error to standard error. Prints the filename, line number,
// column, error message, and the problematic line of code.
void print_parse_error(const char *filename, const struct ParseError *err);

// Prints an evaluation error to standard error. Prints the error message and,
// for certain error types, other information relevant to the error.
void print_eval_error(const struct EvalError *err);

#endif
