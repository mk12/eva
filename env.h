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

// Returns a new, empty environment.
struct Environment *empty_environment(void);

// Returns an environment containing mappings for special procedures.
struct Environment *default_environment(void);

// Looks up an expression in the environment by its key.
struct LookupResult lookup(struct Environment *env, InternID key);

// Binds a new variable in the environment.
void bind(struct Environment *env, InternID key, struct Expression expr);

// Unbinds a variable in the envrionment. The variable must be present.
void unbind(struct Environment *env, InternID key);

#endif
