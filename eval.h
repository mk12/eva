// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

struct Environment;

struct EvalResult {
	struct Expression *expr;
	const char *err_msg;
};

// Evaluates the expression in the given environment. If it can't be evaluated,
// stores NULL in the expr field and provides an error message.
struct EvalResult eval(struct Expression *expr, struct Environment *env);
