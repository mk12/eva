// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef REPL_H
#define REPL_H

#include <stdbool.h>

#include "parse.h"

struct Environment;

// Prefix used when printing error messages.
extern const char *error_prefix;

// This should be called once at the beginning of the program.
void setup_readline(void);

// Reads and parses an s-expression from standard input using GNU Readline
// (without prompts). If the parse is incomplete at the end of a line, waits for
// another line to be entered. If there is leftover input, saves it and starts
// reading from it on the next call.
struct ParseResult read_sexpr(void);

// Executes the given program. Optionally prints the last expression evaluated.
// Upon encountering an error, prints an error messge and returns false.
// Otherwise, returns true.
bool execute(const char *text, struct Environment *env, bool print);

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
