// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

struct Environment {
	Variable var;
	Expression *value;
	Environment *rest;
};

struct Expression *eval(struct Expression *expr, struct Environment *env) {
	switch (tree.type) {
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
