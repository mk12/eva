// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef EVAL_H
#define EVAL_H

#include "expr.h"

struct Environment;

struct EvalResult {
	struct Expression expr;
	struct Environment *env;
	const char *err_msg;
};

// This should be called once at the beginning of the program.
void setup_eval(void);

// Evaluates the expression in the given environment, returning the resulting
// expression and the new environment. If it can't be evaluated, provides an
// error message.
struct EvalResult eval(struct Expression expr, struct Environment *env);

#endif
