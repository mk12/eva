// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "check.h"

#include "error.h"

static const char *check_arg_count(struct Expression proc, size_t n) {
	int arity;
	if (proc.type == E_SPECIAL) {
		arity = special_arity(proc.special_type);
	} else {
		assert(proc.type == E_LAMBDA);
		arity = proc.box->lambda.arity;
	}

	// Return err_arity if the arity doesn't match.
	if (arity >= 0) {
		return n == (size_t)arity ? NULL : err_arity;
	} else {
		return n >= (size_t)(-arity - 1) ? NULL : err_arity;
	}
}

static const char *check_arg_types(
		enum SpecialType type, struct Expression *args, size_t n) {
	switch (type) {
	case S_APPLY:
		if (args[0].type != E_SPECIAL && args[0].type != E_LAMBDA) {
			return err_not_proc;
		}
		if (args[1].type != E_NULL && args[1].type != E_PAIR) {
			return err_not_list;
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
				return err_not_num;
			}
		}
		break;
	case S_DIV:
	case S_REM:
		for (size_t i = 0; i < n; i++) {
			if (args[i].type != E_NUMBER) {
				return err_not_num;
			}
			if (i > 0 && args[i].number == 0) {
				// This technically isn't a type error.
				return err_divide_zero;
			}
		}
		break;
	case S_CAR:
	case S_CDR:
		if (args[0].type != E_PAIR) {
			return err_not_pair;
		}
		break;
	case S_NOT:
		if (args[0].type != E_BOOLEAN) {
			return err_not_bool;
		}
		break;
	default:
		break;
	}
	return NULL;
}

const char *check_application(
		struct Expression proc, struct Expression *args, size_t n) {
	// Check the type of the procedure.
	bool special = proc.type == E_SPECIAL;
	bool lambda = proc.type == E_LAMBDA;
	if (!special && !lambda) {
		return err_op_not_proc;
	}
	// Check the number of arguments.
	const char *err_msg = check_arg_count(proc, n);
	if (err_msg) {
		return err_msg;
	}
	// Check the type of the arguments.
	if (special) {
		err_msg = check_arg_types(proc.special_type, args, n);
	}
	return err_msg;
}

const char *check_list(struct Expression expr) {
	while (expr.type != E_NULL) {
		if (expr.type != E_PAIR) {
			return err_ill_list;
		}
		expr = expr.box->pair.cdr;
	}
	return NULL;
}
