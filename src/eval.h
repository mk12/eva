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

// This should be called once at the beginning of the program.
void setup_eval(void);

// Evaluates a top-level expression in the environment 'env'. On success,
// returns a new expression. Otherwise, allocates and returns an error.
struct EvalResult eval_top(struct Expression expr, struct Environment *env);

#endiu
