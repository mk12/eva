// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

struct Environment;

struct EvalResult {
	struct Expression *expr;
	const char *err_msg;
};

// Returns an environment containing mappings for built-in special functions.
struct Environment *default_environment(void);

// Evaluates the expression in the given environment.
// struct EvalResult eval(struct Expression *expr, struct Environment *env);
struct Expression *eval(struct Expression *expr, struct Environment *env);
