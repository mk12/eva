// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef ENV_H
#define ENV_H

#include "expr.h"

#include <stdbool.h>

struct Environment;

// LookupResult is used to return two values from the lookup function.
struct LookupResult {
	bool found;
	struct Expression expr;
};

// Looks up an expression in the environment by its key.
struct LookupResult lookup(struct Environment *env, unsigned int key);

// Binds a frame of n keys to n expressions. Returns the augmented environment.
struct Environment *bind(
		struct Environment *env,
		const unsigned int *keys,
		const struct Expression *exprs,
		int n);

// Unbinds the most recently bound frame. Returns the reduced environment. Does
// not free the arrays of keys or the array of expressions in the frame.
struct Environment *unbind(struct Environment *env);

// Returns an environment containing mappings for built-in special procedures.
struct Environment *default_environment(void);

#endif
