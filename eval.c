// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"

#include <stdlib.h>

// Evaluation error messages.
static const char *err_not_proc = "operator is not a procedure";
static const char *err_arity = "wrong number of arguments passed to procedure";

struct EvalResult eval(struct Expression *expr, struct Environment *env) {
	struct EvalResult result;
	result.expr = NULL;
	result.err_msg = NULL;

	switch (expr->type) {
	case E_PAIR:
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

	return result;
}

struct EvalResult apply(
		struct Expression *proc,
		struct Expression **args,
		int n,
		struct Environment *env) {
	struct EvalResult result;
	result.expr = NULL;
	result.err_msg = NULL;

	switch (proc->type) {
	case E_LAMBDA:
		if (n != proc->lambda.arity) {
			result.err_msg = err_arity;
			break;
		}
		for (int i = 0; i < n; i++) {
			env = bind(env, proc->lambda.params[i], args[i]);
		}
		result = eval(proc->lambda.body, env);
		unbind(env, n);
		break;
	case E_SPECIAL:
		if (n != special_procs[proc->special.type].arity) {
			result.err_msg = err_arity;
			break;
		}
		switch (proc->special.type) {
		case S_ATOM:
		case S_EQ:
		case S_CAR:
		case S_CDR:
		case S_CONS:
		case S_ADD:;
			int total = 0;
			for (int i = 0; i < n; i++) {
				if (args[i]->type != E_NUMBER) {
					// err_msg
				}
				total += args[i]->number.n;
			}
			result.expr = new_number(total);
		case S_SUB:
		case S_MUL:
		case S_DIV:
			break;
		}
		break;
	default:
		result.err_msg = err_not_proc;
		break;
	}

	return result;
}
