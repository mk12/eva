// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef REPL_H
#define REPL_H

#include <stdbool.h>

struct Environment;

// Executes the given program. Optionally prints the last expression evaluated.
// Upon encountering an error, prints an error messge and returns early.
void execute(const char *text, struct Environment *env, bool print);

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
void repl(struct Environment *env);

#endif
