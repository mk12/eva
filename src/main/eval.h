// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef EVAL_H
#define EVAL_H

#include "expr.h"

struct Environment;

// EvalResult contains the result of evaluating code. The 'expr' field has a
// meaningful value if and only if 'err' is NULL.
struct EvalResult {
	struct Expression expr;
	struct EvalError *err;
};

// Evaluates an expression in the environment 'env'. Definitions (applications
// of F_DEFINE) are only allowed if 'allow_define' is true. On success, returns
// a new expression. Otherwise, allocates and returns an error.
struct EvalResult eval(
		struct Expression expr, struct Environment *env, bool allow_define);

#endif
