// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "type.h"

#include "error.h"
#include "list.h"

#include <assert.h>
#include <stddef.h>

static struct EvalError *check_stdmacro(
		enum StandardMacro stdmacro, struct Expression *args, size_t n) {
	size_t length;
	struct Expression expr;

	switch (stdmacro) {
	case F_DEFINE:
	case F_SET:
		if (args[0].type != E_SYMBOL) {
			return new_eval_error_expr(ERR_TYPE_VAR, args[0]);
		}
		break;
	case F_LAMBDA:
		expr = args[0];
		if (expr.type != E_NULL
				&& expr.type != E_PAIR
				&& expr.type != E_SYMBOL) {
			return new_syntax_error(expr);
		}
		while (expr.type != E_NULL) {
			if (expr.type == E_PAIR) {
				if (expr.box->car.type != E_SYMBOL) {
					return new_eval_error_expr(ERR_TYPE_VAR, expr.box->car);
				}
			} else if (expr.type == E_SYMBOL) {
				break;
			} else {
				return new_eval_error_expr(ERR_TYPE_VAR, expr);
			}
			expr = expr.box->cdr;
		}
		break;
	case F_COND:
		for (size_t i = 0; i < n; i++) {
			if (!count_list(&length, args[i]) || length < 2) {
				return new_syntax_error(args[i]);
			}
		}
		break;
	case F_LET:
	case F_LET_STAR:
	case F_LET_REC:
		expr = args[0];
		while (expr.type != E_NULL) {
			if (expr.type == E_PAIR) {
				if (!count_list(&length, expr.box->car) || length != 2) {
					return new_syntax_error(expr.box->car);
				}
				if (expr.box->car.box->car.type != E_SYMBOL) {
					return new_eval_error_expr(
							ERR_TYPE_VAR, expr.box->car.box->car);
				}
			} else {
				return new_syntax_error(args[0]);
			}
			expr = expr.box->cdr;
		}
		break;
	default:
		break;
	}
	return NULL;
}

static struct EvalError *check_stdproc(
		enum StandardProcedure stdproc, struct Expression *args, size_t n) {
	switch (stdproc) {
	case S_APPLY:;
		Arity arity;
		if (!expression_arity(&arity, args[0])) {
			return new_eval_error_expr(ERR_TYPE_OPERATOR, args[0]);
		}
		size_t length;
		if (!count_list(&length, args[n-1])) {
			return new_syntax_error(args[n-1]);
		}
		size_t n_args = length + n - 2;
		if (!arity_allows(arity, n_args)) {
			return new_arity_error(arity, n_args);
		}
		break;
	case S_MACRO:
		if (args[0].type != E_STDPROCEDURE && args[0].type != E_PROCEDURE) {
			return new_type_error(E_PROCEDURE, args, 0);
		}
		break;
	case S_NUM_EQ:
	case S_NUM_LT:
	case S_NUM_GT:
	case S_NUM_LE:
	case S_NUM_GE:
	case S_ADD:
	case S_SUB:
	case S_MUL:
	case S_EXPT:
		for (size_t i = 0; i < n; i++) {
			if (args[i].type != E_NUMBER) {
				return new_type_error(E_NUMBER, args, i);
			}
		}
		break;
	case S_DIV:
	case S_REM:
	case S_MOD:
		for (size_t i = 0; i < n; i++) {
			if (args[i].type != E_NUMBER) {
				return new_type_error(E_NUMBER, args, i);
			}
			if (i > 0 && args[i].number == 0) {
				// This is not technically a type error, but this is the
				// earliest and most convenient place to catch it.
				return new_eval_error(ERR_DIV_ZERO);
			}
		}
		break;
	case S_NOT:
		if (args[0].type != E_BOOLEAN) {
			return new_type_error(E_BOOLEAN, args, 0);
		}
		break;
	case S_CAR:
	case S_CDR:
	case S_SET_CAR:
	case S_SET_CDR:
		if (args[0].type != E_PAIR) {
			return new_type_error(E_PAIR, args, 0);
		}
		break;
	default:
		break;
	}
	return NULL;
}

struct EvalError *type_check(
		struct Expression expr, struct Expression *args, size_t n) {
	switch (expr.type) {
	case E_STDMACRO:
		return check_stdmacro(expr.stdmacro, args, n);
	case E_STDPROCEDURE:
		return check_stdproc(expr.stdproc, args, n);
	case E_MACRO:
	case E_PROCEDURE:
		break;
	default:
		assert(false);
		break;
	}
	return NULL;
}
