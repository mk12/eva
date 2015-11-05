// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "env.h"

#include "expr.h"

#include <stdlib.h>
#include <string.h>

// An Environment maps variables to values. It is implemented as a linked list.
// It has no memory ownership of its variable strings or its expressions.
struct Environment {
	const char *var;
	struct Expression *val;
	struct Environment *rest;
};

struct Expression *lookup(struct Environment *env, const char *var) {
	while (env) {
		if (strcmp(var, env->var) == 0) {
			return env->val;
		}
		env = env->rest;
	}
	return NULL;
}

struct Environment *bind(
		struct Environment *env,
		const char **vars,
		struct Expression **vals,
		int n) {
	for (int i = 0; i < n; i++) {
		struct Environment *head = malloc(sizeof *head);
		head->var = vars[i];
		head->val = vals[i];
		head->rest = env;
		env = head;
	}
	return env;
}

struct Environment *unbind(struct Environment *env, int n) {
	for (int i = 0; i < n; i++) {
		struct Environment *temp = env;
		env = env->rest;
		free(temp);
	}
	return env;
}

struct Environment *default_environment(void) {
	struct Environment *env = NULL;
	for (int i = 0; i < N_SPECIAL_PROCS; i++) {
		env = bind(env, special_procs[i].name, new_special(i));
	}
	return env;
}
