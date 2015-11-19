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

// Evaluates the expression in the given environment. If it can't be evaluated,
// stores NULL in the expr field and provides an error message.
struct EvalResult eval(struct Expression expr, struct Environment *env);

#endif
