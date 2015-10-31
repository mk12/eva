// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

// An Environment maps variables to values. The environment owns all its
// variable strings, but it does not own the expressions.
struct Environment {
	char *var;
	struct Expression *val;
	struct Environment *rest;
};

// Looks up a variable in the environment. Returns NULL if it doesn't appear.
struct Expression *lookup(struct Environment *env, const char *var) {
	while (env) {
		if (strcmp(var.name, env->var.name) == 0) {
			return env->val;
		}
		env = env->rest;
	}
	return NULL;
}

// Binds a variable to an expression in the environment. Returns the augmented
// environment.
struct Environment *bind(
		struct Environment *env, char *var, struct Expression *val) {
	struct Environment *head = malloc(sizeof *head);
	head->var = var;
	head->val = val;
	head->rest = env;
	return head;
}

// Unbinds the most recently bound variable in the environment. Returns the
// reduced environment.
struct Environment *unbind(struct Environment *env) {
	struct Environment *rest = env->rest;
	free(env->var);
	free(env);
	return rest;
}

struct Expression *eval(struct Expression *expr, struct Environment *env) {
	switch (expr->type) {
	case E_NULL:

	case T_NODE:
		// Check node's first child (IF IT IS A LEAF):
		// (SPECIAL FORMS)
		// - "quote" -> ...
		// - "lambda" -> ...
		// - "cond"   -> ...
		// - "define" -> ...
		// otherwise, eval everyting + call apply
		break;
	case T_LEAF:
		// "1" -> E_NUMBER 1
		// "a" -> lookup in env, or error
		// prepopulate env with "car", "cdr", "+", etc.
		// (or check here)
		break;
	}
}

struct Expression *quote(struct Expression *expr) {
	switch (tree.type) {
	case T_NODE:
		// build a list
		// cons 1st (const 2nd (const .. E_NULL))
		break;
	case T_LEAF:
		// "1" -> E_NUMBER 1
		// "a" -> E_SYMBOL
		break;
	}
}

struct Expression *apply(
		struct Expression *proc,
		struct Expression *args,
		int n,
		struct Environment *env) {
	if (proc.type == E_LAMBDA) {
		assert(proc.lambda.arity == n);
		for (int i = 0; i < n; i++) {
			// add proc.lambda.params[i] -> args[i] to env
		}
		Expression *result = eval(body, env);
		// remove bindings from env
		return result;
	}

	assert(proc.type == E_SPECIAL);
	switch (proc.special.id) {
	case S_ADD:
		int total = 0;
		for (int i = 0; i < n; i++) {
			assert(args[i].type == E_NUMBER);
			total += args[i].number.n;
		}
		// return new expression E_NUMBER with n=total
		break;
	case S_SUB:
		break;
	}
}
