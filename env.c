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
		struct Environment *env, const char *var, struct Expression *val) {
	struct Environment *head = malloc(sizeof *head);
	head->var = var;
	head->val = val;
	head->rest = env;
	return head;
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
	for (int i = 0; i < n_special_names; i++) {
		env = bind(env, special_names[i], new_special((enum SpecialType)i));
	}
	return env;
}
