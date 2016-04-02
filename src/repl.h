// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef REPL_H
#define REPL_H

#include <stdbool.h>

struct Environment;
struct ParseError;

// This should be called once at the beginning of the program.
void setup_readline(void);

// Reads and parses an s-expression from standard input using GNU Readline
// (without prompts). If the parse is incomplete at the end of a line, waits for
// another line to be entered. If there is leftover input, saves it and starts
// reading from it on the next call.
//
// On success, stores the parsed expression in 'out' and returns NULL.
// Otherwise, allocates and returns a parse error.
struct ParseError *read_sexpr(struct Expression *out);

// Executes the given program. Optionally prints the last expression evaluated.
// Upon encountering an error, prints an error messge and returns false.
// Otherwise, returns true. The filename is only used for error messages.
bool execute(
	const char *filename,
	const char *text,
	struct Environment *env,
	bool print);

// Runs the Read-Eval-Print Loop. Each iteration has five steps:
//
// 1. Present the promp "eva> ".
// 2. Parse a line of input read by GNU Readline.
// 3. Read and parse more lines if necessary.
// 4. Evaluate the code.
// 5. Print the resulting expression or an error message.
//
// Updates the environment with top-level defintions as they are encountered.
// Returns upon EOF (Ctrl-D) or SIGINT (Ctrl-C).
//
// If print is false, skips steps 1 and 5.
void repl(struct Environment *env, bool print);

#endif
