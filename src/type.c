// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "type.h"

#include "error.h"

static struct EvalError *check_arg_count(struct Expression proc, size_t n) {
	// Get the arity of the procedure.
	int arity;
	if (proc.type == E_SPECIAL) {
		arity = special_arity(proc.special_type);
	} else {
		assert(proc.type == E_LAMBDA);
		arity = proc.box->lambda.arity;
	}

	// Check if the arguments match the sign-encoded arity.
	if ((arity >= 0 && n == (size_t)arity)
			|| (arity < 0 && n >= (size_t)(-(arity+1)))) {
		return NULL;
	}

	// Allocate and return an error.
	struct EvalError *err = new_eval_error(ERR_ARITY);
	err->arity = arity;
	err->n_args= n;
	return err;
}

static struct EvalError *check_arg_types(
		enum SpecialType type, struct Expression *args, size_t n) {
	switch (type) {
	case S_APPLY:
		if (args[0].type != E_SPECIAL && args[0].type != E_LAMBDA) {
			struct EvalError *err = new_eval_error(ERR_NOT_PROC);
			err->expr = args[0];
			return err;
		}
		// The second argument must be a well-formed list. The evaluator will
		// catch it if it's not, though, so here we only check the basic type.
		if (args[1].type != E_NULL && args[1].type != E_PAIR) {
			return new_eval_error(ERR_PROC_CALL);
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
		for (size_t i = 0; i < n; i++) {
			if (args[i].type != E_NUMBER) {
				return new_type_error(E_NUMBER, i, args[i]);
			}
		}
		break;
	case S_DIV:
	case S_REM:
		for (size_t i = 0; i < n; i++) {
			if (args[i].type != E_NUMBER) {
				return new_type_error(E_NUMBER, i, args[i]);
			}
			if (i > 0 && args[i].number == 0) {
				// This is not technically a type error, but this is the
				// earliest and most convenient place to catch it.
				return new_eval_error(ERROR_DIV_ZERO);
			}
		}
		break;
	case S_CAR:
	case S_CDR:
		if (args[0].type != E_PAIR) {
			return new_type_error(E_PAIR, i, args[i]);
		}
		break;
	case S_NOT:
		if (args[0].type != E_BOOLEAN) {
			return new_type_error(E_BOOLEAN, i, args[i]);
		}
		break;
	default:
		break;
	}
	return NULL;
}

struct EvalError *type_check(
		struct Expression proc, struct Expression *args, size_t n) {
	// Check the type of the procedure.
	bool special = proc.type == E_SPECIAL;
	bool lambda = proc.type == E_LAMBDA;
	if (!special && !lambda) {
		struct EvalError *err = new_eval_error(ERR_NOT_PROC);
		err->expr = proc;
		return err;
	}
	// Check the number of arguments.
	struct EvalError *err = check_arg_count(proc, n);
	if (err) {
		return err;
	}
	// Check the type of the arguments.
	if (special) {
		err = check_arg_types(proc.special_type, args, n);
	}
	return err;
}
