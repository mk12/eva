// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef REPL_H
#define REPL_H

struct Environment;

// Runs the Read-Eval-Print Loop:
//
// 1. Present the promp "eva> ".
// 2. Read a line of input using GNU Readline, and parse it.
// 3. Read and parse more lines of input if necessary.
// 4. Evaluate the code.
// 5. Print the resulting expression.
// 6. Repeat.
//
// Updates the environment with top-level defintions as they are encountered.
// Exits upon EOF (Ctrl-D) or SIGINT (Ctrl-C).
void repl(struct Environment *env);

// Executes the given program. Unlike the REPL, does not print anything unless
// the program contains printing commands.
void execute(const char *text, struct Environment *env);

#endif
