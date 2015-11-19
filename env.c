// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "env.h"

#include "expr.h"
#include "intern.h"

#include <stdlib.h>
#include <string.h>

// An Environment is a linked list of frames. Each frame maps a set of integer
// keys to expressions.
struct Environment {
	int n_keys;
	const unsigned int *keys;
	const struct Expression *exprs;
	struct Environment *rest;
};

struct LookupResult lookup(struct Environment *env, unsigned int key) {
	while (env) {
		for (int i = 0; i < env->n_keys; i++) {
			if (env->keys[i] == key) {
				struct Expression expr = env->exprs[i];
				return (struct LookupResult){ .found = true, .expr = expr };
			}
		}
		env = env->rest;
	}
	return (struct LookupResult){ .found = false };
}

struct Environment *bind(
		struct Environment *env,
		const unsigned int *keys,
		const struct Expression *exprs,
		int n) {
	struct Environment *head = malloc(sizeof *head);
	head->n_keys = n;
	head->keys = keys;
	head->exprs = exprs;
	return env;
}

struct Environment *unbind(struct Environment *env) {
	struct Environment *tail = env->rest;
	free(env);
	return tail;
}

struct Environment *default_environment(void) {
	static unsigned int keys[N_SPECIAL_PROCS];
	static struct Expression exprs[N_SPECIAL_PROCS];
	static bool first = true;

	if (first) {
		for (int i = 0; i < N_SPECIAL_PROCS; i++) {
			keys[i] = intern_string(special_name(i));
			exprs[i] = new_special(i);
		}
		first = false;
	}

	return bind(NULL, keys, exprs, N_SPECIAL_PROCS);
}
