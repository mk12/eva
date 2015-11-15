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

// TODO: array of macro names (so you can't (define lambda 5)).

// Returns NULL if the procedure accepts n arguments. Returns an error message
// otherwise. Assumes proc is a lambda expression or special expression.
static const char *check_arg_count(struct Expression *proc, int n) {
	int arity;
	if (proc->type == E_LAMBDA) {
		arity = proc->lambda.arity;
	} else {
		assert(proc->type == E_SPECIAL);
		arity = special_procs[proc->special.type].arity;
	}

	if (arity >= 0) {
		return n == arity;
	} else {
		return n >= -(arity + 1);
	}
}

// Returns NULL if all argument are of the correct type. Returns an error
// message otherwise. Assumes the correct number of arguments are provided.
static const char *check_arg_types(
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

// Returns NULL if the application of proc to args is valid. Returns an error
// message otherwise (including the case where proc is not a procedure).
static const char *check_application(
		struct Expression *proc, struct Expression **args, int n) {
	bool lambda = proc->type == E_LAMBDA;
	bool special = proc->type == E_SPECIAL;
	if (!lambda && !special) {
		return err_not_proc;
	}

	const char *err_msg = check_arg_count(proc, n);
	if (err_msg) {
		return err_msg;
	}
	if (special) {
		err_msg = check_arg_types(proc->special.type, args, n);
	}
	return err_msg;
}

// Returns the expression resulting from applying a special procedure to
// arguments. Assumes the arguments are correct in number and in types.
static struct Expression *apply_special(
		enum SpecialType type, struct Expression **args, int n) {
	switch (type) {
	case S_NULL:
		return new_boolean(args[0]->type == E_NULL);
	case S_PAIR:
		return new_boolean(args[0]->type == E_PAIR);
	case S_NUMBER:
		return new_boolean(args[0]->type == E_NUMBER);
	case S_BOOLEAN:
		return new_boolean(args[0]->type == E_BOOLEAN);
	case S_PROCEDURE:;
		enum ExpressionType t = args[0]->type;
		return new_boolean(t == E_LAMBDA || t == E_SPECIAL);
	case S_EQ:
		return args[0] == args[1];
	case S_NUM_EQ:
		return new_boolean(args[0]->number.n == args[1]->number.n);
	case S_LT:
		return new_boolean(args[0]->number.n < args[1]->number.n);
	case S_GT:
		return new_boolean(args[0]->number.n > args[1]->number.n);
	case S_LE:
		return new_boolean(args[0]->number.n <= args[1]->number.n);
	case S_GE:
		return new_boolean(args[0]->number.n >= args[1]->number.n);
	case S_CONS:
		return new_pair(args[0], args[1]);
	case S_CAR:
		return args[0]->pair.car;
	case S_CDR:
		return args[0]->pair.cdr;
	case S_ADD:;
		int sum = 0;
		for (int i = 0; i < n; i++) {
			sum += args[i]->number.n;
		}
		return new_number(sum);
	case S_SUB:
		if (n == 1) {
			return new_number(-args[0]->number.n);
		}
		int diff = args[0]->number.n;
		for (int i = 1; i < n; i++) {
			diff -= args[i]->number.n;
		}
		return new_number(diff);
	case S_MUL:;
		int prod = 0;
		for (int i = 0; i < n; i++) {
			prod *= args[i]->number.n;
		}
		return new_number(prod);
	case S_DIV:
		if (n == 1) {
			result.expr = new_number(1 / args[0]->number.n);
			break;
		}
		int quot = args[0]->number.n;
		for (int i = 1; i < n; i++) {
			quot /= args[i]->number.n;
		}
		return new_number(quot);
	case S_REM:;
		int rem = args[0]->number.n % args[1]->number.n;
		return new_number(rem);
	case S_NOT:
		return new_boolean(!args[0]->boolean.b);
	}
}

// Applies a procedure to arguments.
static struct EvalResult apply(
		struct Expression *proc,
		struct Expression **args,
		int n,
		struct Environment *env) {
	struct EvalResult result;
	result.expr = NULL;
	result.err_msg = check_application(proc, args, n);
	if (result.err_msg) {
		return result;
	}

	if (proc->type == E_LAMBDA) {
		env = bind(env, proc->lambda.params, args, n);
		result = eval(proc->lambda.body, env);
		unbind(env, n);
		return result;
	}

	assert(proc->type == E_SPECIAL);
	result.expr = apply_special(proc->special.type, args, n);
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
