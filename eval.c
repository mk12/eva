// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"

#include <stdlib.h>

struct EvalResult eval(struct Expression *expr, struct Environment *env) {
	struct EvalResult result;
	result.expr = NULL;
	result.err_msg = NULL;
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
		break;
	default:
		result.expr = clone_expression(expr);
		break;
	}
}

struct EvalResult apply(
		struct Expression *proc,
		struct Expression **args,
		int n,
		struct Environment *env) {
	struct EvalResult result;
	result.expr = NULL;
	result.err_msg = NULL;

	if (proc->type == E_LAMBDA) {
		/* assert(proc.lambda.arity == n); */
		// error: wrong number of arguments
		for (int i = 0; i < n; i++) {
			env = bind(env, proc->lambda.params[i], args[i]);
		}
		struct EvalResult res = eval(proc->lambda.body, env);
		env = unbind(env, n);
		if (!res.expr) {
			result.err_msg = res.err_msg;
		} else {
			result.expr = res.expr;
		}
		return result;
	}

	/* assert(proc->type == E_SPECIAL); */
	switch (proc->special.type) {
	case S_ATOM:
	case S_EQ:
	case S_CAR:
	case S_CDR:
	case S_CONS:
	case S_ADD:;
		int total = 0;
		for (int i = 0; i < n; i++) {
			/* assert(args[i].type == E_NUMBER); */
			// error: wrong type
			total += args[i]->number.n;
		}
		result.expr = new_number(total);
	case S_SUB:
	case S_MUL:
	case S_DIV:
		break;
	}

	return result;
}
