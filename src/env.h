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

// Creates a new base environment: an environment with no parent.
struct Environment *new_base_environment(void);

// Creates a new evironment with the given parent environment. Retains the
// parent and sets the reference count of the new environment to 1. The size
// estimate refers to the total number of bindings the environment will hold.
struct Environment *new_environment(
		struct Environment *parent, size_t size_estimate);

// Increments the reference count of the environment. This is a no-op if 'env'
// is NULL. Returns the environment for convenience.
struct Environment *retain_environment(struct Environment *env);

// Decrements the reference count of the environment. This is a no-op if 'env'
// is NULL. Deallocates the environment, which includes releasing its parent and
// all its bound expressions, if the reference count reaches zero.
void release_environment(struct Environment *env);

// Looks up the expression bound to 'key' in the environment. If it can't be
// found, searches in its parent environment, then in the parent's parent, etc.
struct LookupResult lookup(const struct Environment *env, InternId key);

// Binds 'key' to 'expr' in the environment, retaining 'expr'. If 'key' has
// previously been bound in the environment (not including its parents), this
// overwrites the old expression.
void bind(struct Environment *env, InternId key, struct Expression expr);

#endif
