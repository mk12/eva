// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"

#include <stdlib.h>

// Evaluation error messages.
static const char *err_not_proc = "operator is not a procedure";
static const char *err_arity = "wrong number of arguments passed to procedure";
static const char *err_not_num = "expected operand to be a number";
static const char *err_not_bool = "expected operand to be a boolean";
static const char *err_not_pair = "expected operand to be a pair";

// If all types are correct, returns NULL. If one or more arguments are of the
// wrong type, returns an error message. Assumes the correct number of arguments
// have been provided.
static const char *typecheck_args(
		enum SpecialType type, struct Expression **args, int n) {
	switch (type) {
	case S_CAR:
	case S_CDR:
		if (args[0]->type != E_PAIR) {
			return err_not_pair;
		}
		break;
	case S_NUM_EQ:
	case S_LT:
	case S_GT:
	case S_LE:
	case S_GE:
	case S_ADD:
	case S_SUB:
	case S_MUL:
	case S_DIV:
	case S_REM:
		for (int i = 0; i < n; i++) {
			if (args[i]->type != E_NUMBER) {
				return err_not_num;
			}
		}
		break;
	case S_NOT:
		if (args[0]->type != E_BOOLEAN) {
			return err_not_bool;
		}
		break;
	default:
		break;
	}
	return NULL;
}

static struct EvalResult apply(
		struct Expression *proc,
		struct Expression **args,
		int n,
		struct Environment *env) {
	struct EvalResult result;
	result.expr = NULL;
	result.err_msg = NULL;

	switch (proc->type) {
	case E_LAMBDA:
		if (!accepts_args(proc, n)) {
			result.err_msg = err_arity;
			break;
		}
		// check if negative arity
		for (int i = 0; i < n; i++) {
			env = bind(env, proc->lambda.params[i], args[i]);
		}
		result = eval(proc->lambda.body, env);
		unbind(env, n);
		break;
	case E_SPECIAL:
		if (!accepts_args(proc, n)) {
			result.err_msg = err_arity;
			break;
		}
		result.err_msg = typecheck_args(proc->special.type, args, n);
		if (result.err_msg) {
			break;
		}
		switch (proc->special.type) {
		case S_NULL:
			result.expr = new_boolean(args[0]->type == E_NULL);
			break;
		case S_PAIR:
			result.expr = new_boolean(args[0]->type == E_PAIR);
			break;
		case S_NUMBER:
			result.expr = new_boolean(args[0]->type == E_NUMBER);
			break;
		case S_BOOLEAN:
			result.expr = new_boolean(args[0]->type == E_BOOLEAN);
			break;
		case S_PROCEDURE:;
			enum ExpressionType t = args[0]->type;
			result.expr = new_boolean(t == E_LAMBDA || t == E_SPECIAL);
			break;
		case S_EQ:
		case S_NUM_EQ:
			result.expr = new_boolean(args[0]->number.n == args[1]->number.n);
			break;
		case S_LT:
			result.expr = new_boolean(args[0]->number.n < args[1]->number.n);
			break;
		case S_GT:
			result.expr = new_boolean(args[0]->number.n > args[1]->number.n);
			break;
		case S_LE:
			result.expr = new_boolean(args[0]->number.n <= args[1]->number.n);
			break;
		case S_GE:
			result.expr = new_boolean(args[0]->number.n >= args[1]->number.n);
			break;
		case S_CONS:
		case S_CAR:
		case S_CDR:
		case S_ADD:;
			int sum = 0;
			for (int i = 0; i < n; i++) {
				sum += args[i]->number.n;
			}
			result.expr = new_number(sum);
			break;
		case S_SUB:
			if (n == 1) {
				result.expr = new_number(-args[0]->number.n);
				break;
			}
			int diff = args[0]->number.n;
			for (int i = 1; i < n; i++) {
				diff -= args[i]->number.n;
			}
			result.expr = new_number(diff);
			break;
		case S_MUL:;
			int prod = 0;
			for (int i = 0; i < n; i++) {
				prod *= args[i]->number.n;
			}
			result.expr = new_number(prod);
			break;
		case S_DIV:
			if (n == 1) {
				result.expr = new_number(1 / args[0]->number.n);
				break;
			}
			int quot = args[0]->number.n;
			for (int i = 1; i < n; i++) {
				quot /= args[i]->number.n;
			}
			result.expr = new_number(quot);
			break;
		case S_REM:;
			int rem = args[0]->number.n % args[1]->number.n;
			result.expr = new_number(rem);
			break;
		case S_NOT:
			result.expr = new_boolean(!args[0]->boolean.b);
			break;
		}
		break;
	default:
		result.err_msg = err_not_proc;
		break;
	}

	return result;
}

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
		// otherwise, eval everything + call apply
		// think about memory management
		break;
	case E_SYMBOL:
		// "1" -> E_NUMBER 1
		// "a" -> lookup in env, or error
		// prepopulate env with "car", "cdr", "+", etc.
		// (or check here)
		break;
	default:
		// Everything else is self-evaluating.
		result.expr = clone_expression(expr);
		break;
	}

	return result;
}
