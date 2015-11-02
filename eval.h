// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

struct Environment;

// Returns an environment containing mappings for built-in special functions.
struct Environment *default_environment(void);

// Evaluates the expression in the given environment.
struct Expression *eval(struct Expression *expr, struct Environment *env);
