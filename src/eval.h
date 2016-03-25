// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef EVAL_H
#define EVAL_H

#include "expr.h"

struct Environment;

struct EvalResult {
	struct Expression expr;
	const char *err_msg;
};

// This should be called once at the beginning of the program.
void setup_eval(void);

// Evaluates a top-level expression in the given environment, returning a new
// expression. Modifies the environment if the expression is a definition. If
// the expression can't be evaluated, provides an error message.
struct EvalResult eval_top(struct Expression expr, struct Environment *env);

#endif
