// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

// An Environment maps variables to values. The environment owns neither the
// variable strings nor the expressions (it doesn't free them).
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

// Unbinds the n most recently bound variables in the environment. Returns the
// reduced environment.
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
		env = bind(env, special_names[i], (enum SpecialType)i);
	}
	return env;
}

struct Expression *eval(struct Expression *expr, struct Environment *env) {
	switch (expr->type) {
	case E_CONS:
		// CHECK SPECIAL FORMS
		// - "quote" -> ...
		// - "lambda" -> ...
		// - "cond"   -> ...
		// - "define" -> ...
		// otherwise, eval everyting + call apply
		break;
	case E_SYMBOL:
		// "1" -> E_NUMBER 1
		// "a" -> lookup in env, or error
		// prepopulate env with "car", "cdr", "+", etc.
		// (or check here)
	default:
		return clone_expression(expr):
	}
}

struct Expression *apply(
		struct Expression *proc,
		struct Expression *args,
		int n,
		struct Environment *env) {
	if (proc.type == E_LAMBDA) {
		assert(proc.lambda.arity == n);
		// error: wrong number of arguments
		for (int i = 0; i < n; i++) {
			env = bind(env, expr->lambda.params[i], args[i]);
		}
		Expression *result = eval(body, env);
		env = unbind(env, n);
		return result;
	}

	assert(proc.type == E_SPECIAL);
	switch (proc.special.id) {
	case S_ATOM:
	case S_EQ:
	case S_CAR:
	case S_CDR:
	case S_CONS:
	case S_ADD:;
		int total = 0;
		for (int i = 0; i < n; i++) {
			assert(args[i].type == E_NUMBER);
			// error: wrong type
			total += args[i].number.n;
		}
		return new_number(total);
	case S_SUB:
	case S_MUL:
	case S_DIV:
	case S_SUB:
		break;
	}
}
