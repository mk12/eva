// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "env.h"

#include "expr.h"

#include <stdlib.h>
#include <string.h>

// An Environment maps variables (integer identifiers) to expressions. It is
// implemented as a linked list.
struct Environment {
	int id;
	struct Expression expr;
	struct Environment *rest;
};

struct Expression lookup(struct Environment *env, int id) {
	while (env) {
		if (env->id == id) {
			return env->expr;
		}
		env = env->rest;
	}
	return NULL;
}

struct Environment *bind(
		struct Environment *env,
		int *ids,
		struct Expression *exprs,
		int n) {
	for (int i = 0; i < n; i++) {
		struct Environment *head = malloc(sizeof *head);
		head->id = ids[i];
		head->expr = exprs[i];
		head->rest = env;
		env = head;
		// reference count
	}
	return env;
}

struct Environment *unbind(struct Environment *env, int n) {
	for (int i = 0; i < n; i++) {
		struct Environment *temp = env;
		env = env->rest;
		free(temp);
		// reference count
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
