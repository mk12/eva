// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef ENV_H
#define ENV_H

#include "expr.h"

#include <stdbool.h>

struct Environment;
struct Expression;

// LookupResult is used to return two values from the lookup function.
struct LookupResult {
	bool found;
	struct Expression expr;
};

// Looks up a variable in the environment by its identifier.
struct LookupResult lookup(struct Environment *env, int id);

// Binds n variables to n expressions. Returns the augmented environment.
struct Environment *bind(
		struct Environment *env, int *ids, struct Expression *exprs, int n);

// Unbinds the n most recently bound variables. Returns the reduced environment.
struct Environment *unbind(struct Environment *env, int n);

// Returns an environment containing mappings for built-in special functions.
struct Environment *default_environment(void);

#endif
