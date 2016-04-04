// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef ENV_H
#define ENV_H

#include "expr.h"

#include <stdbool.h>

struct Environment;

// LookupResult is used to return two values from the lookup function. The
// 'expr' field has a meaningful value if and only if 'found' is true.
struct LookupResult {
	bool found;
	struct Expression expr;
};

// Returns a new, empty environment.
struct Environment *empty_environment(void);

// Returns an environment containing mappings for standard procedures.
struct Environment *default_environment(void);

// Looks up an expression in the environment by its key.
struct LookupResult lookup(struct Environment *env, InternId key);

// Binds a new variable in the environment, retaining the expression.
void bind(struct Environment *env, InternId key, struct Expression expr);

// Unbinds a variable in the environment, releasing the expression. If the key
// is not found in the environment, this is a no-op.
void unbind(struct Environment *env, InternId key);

// Like unbind, but more efficient. Requires the key to correspond to the most
// recently added variable.
void unbind_last(struct Environment *env, InternId key);

// Frees all memory used by the environment and releases bound expressions.
void free_environment(struct Environment *env);

#endif
