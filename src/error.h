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
	ERR_DUP_PARAM,      // code, symbol_id
	ERR_NON_EXHAUSTIVE, // code
	ERR_READ,           // parse_err
	ERR_SYNTAX,         // code
	ERR_TYPE_OPERAND,   // code, expected_type, expr, arg_pos
	ERR_TYPE_OPERATOR,  // code, expr
	ERR_TYPE_VAR,       // code, expr
	ERR_UNBOUND_VAR     // code, symbol_id
};

// An error that causes the parse to fail. Errors created on the stack usually
// use the 'text' field. Errors allocated by 'new_parse_error' use 'owned_text',
// which is freed upon calling 'free_parse_error'.
struct ParseError {
	enum ParseErrorType type;
	union {
		const char *text; // full text being parsed
		char *owned_text; // same thing, but the error owns the memory
	};
	size_t index; // index in 'text' or 'owned_text' where the error occurred
};

// A runtime error that occurs during code evaluation. These are usually created
// with 'has_code' set to false, and later (further up the call stack) the
// evaluator attaches the offending code using 'attach_code' before printing the
// error. Syntax errors are the exception: they usually attach code immediately.
struct EvalError {
	enum EvalErrorType type;
	bool has_code;
	struct Expression code; // context of the error
	union {
		// Used by ERR_DUP_PARAM and ERR_UNBOUND_VAR:
		InternId symbol_id;
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
			struct Expression expr;
			size_t arg_pos;
		};
	};
};

// Constructors for parse errors and evaluation errors.
struct ParseError *new_parse_error(
		enum ParseErrorType type, char *owned_text, size_t index);
struct EvalError *new_eval_error(enum EvalErrorType type);
struct EvalError *new_eval_error_symbol(
		enum EvalErrorType type, InternId symbol_id);
struct EvalError *new_eval_error_expr(
		enum EvalErrorType type, struct Expression expr);
struct EvalError *new_arity_error(Arity arity, size_t n_args);
struct EvalError *new_syntax_error(struct Expression code);
struct EvalError *new_type_error(
		enum ExpressionType expected_type,
		const struct Expression *args,
		size_t arg_pos);

// Retains 'code' and stores it in 'err'. Returns 'err' for convenience.
struct EvalError *attach_code(struct EvalError *err, struct Expression code);

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
