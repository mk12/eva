// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

struct Environment;

// Evaluates the expression in the given environment.
struct Expression *eval(struct Expression *expr, struct Environment *env);
