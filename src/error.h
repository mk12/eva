// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef ERROR_H
#define ERROR_H

// Error types for parse errors.
enum ParseErrorType {
	ERR_EXPECTED_RPAREN,
	ERR_INVALID_DOT,
	ERR_INVALID_LITERAL,
	ERR_UNEXPECTED_EOI,
	ERR_UNEXPECTED_RPAREN
};

// Error types for evaluation errors..
enum EvalErrorType {
	ERR_ARITY,
	ERR_DIV_ZERO,
	ERR_DUP_PARAM,
	ERR_SYNTAX,
	ERR_NON_EXHAUSTIVE,
	ERR_OPERAND,
	ERR_OPERATOR,
	ERR_SPECIAL_VAR,
	ERR_UNBOUND_VAR
};

// An error that occurs during file manipulation.
struct FileError {
	int errno;             // errno value set by the IO function
	const char *filename;  // name of the file being manipulated
};

// An error that causes the parse to fail.
struct ParseError {
	enum ParseErrorType type;
	const char *filename; // filename or description of source
	const char *text;     // full text being parsed
	size_t position;      // index in text where the error occurred
};

// A runtime error that occurs during code evaluation.
struct EvalError {
	enum EvalErrorType type;
	union {
		const char *name;       // a name relevant to the error
		struct Expression expr; // an expression relevant to the error
		InternId symbol_id;     // a symbol relevant to the error
		struct {
			size_t actual; // number of arguments provided
			int expected;  // expected arity
		} arity;
	};
};

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
