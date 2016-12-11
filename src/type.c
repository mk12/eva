// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "type.h"

#include "error.h"
#include "list.h"
#include "set.h"

#include <assert.h>
#include <stddef.h>

// Checks that expression number 'i' has type 't', and returns an error if not.
#define CHECK_TYPE(t, i) \
	if (args[i].type != t) { return new_type_error(t, args, i); }

// Checks that the expression number 'j' (a number) is within the range for
// expression number 'i' (a string).
#define CHECK_RANGE(i, j) \
	if (args[j].number < 0 || args[j].number > (Number)args[i].box->len) { \
		return new_eval_error_expr(ERR_RANGE, args[j]); \
	}

static struct EvalError *check_stdmacro(
		enum StandardMacro stdmacro, struct Expression *args, size_t n) {
	size_t length;
	struct Expression expr;
	struct Set *set;

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
		set = new_set();
		while (expr.type != E_NULL) {
			InternId symbol_id;
			if (expr.type == E_PAIR) {
				if (expr.box->car.type != E_SYMBOL) {
					free_set(set);
					return new_eval_error_expr(ERR_TYPE_VAR, expr.box->car);
				}
				symbol_id = expr.box->car.symbol_id;
			} else if (expr.type == E_SYMBOL) {
				symbol_id = expr.symbol_id;
			} else {
				free_set(set);
				return new_eval_error_expr(ERR_TYPE_VAR, expr);
			}
			if (!add_to_set(set, symbol_id)) {
				free_set(set);
				return attach_code(
						new_eval_error_symbol(ERR_DUP_PARAM, symbol_id),
						args[0]);
			}
			if (expr.type == E_SYMBOL) {
				break;
			}
			expr = expr.box->cdr;
		}
		free_set(set);
		break;
	case F_UNQUOTE:
	case F_UNQUOTE_SPLICING:
		return new_eval_error(ERR_UNQUOTE);
	case F_COND:
		for (size_t i = 0; i < n; i++) {
			if (!count_list(&length, args[i]) || length < 2) {
				return new_syntax_error(args[i]);
			}
		}
		break;
	case F_LET:
	case F_LET_STAR:
		expr = args[0];
		set = new_set();
		while (expr.type != E_NULL) {
			if (expr.type == E_PAIR) {
				if (!count_list(&length, expr.box->car) || length != 2) {
					free_set(set);
					return new_syntax_error(expr.box->car);
				}
				if (expr.box->car.box->car.type != E_SYMBOL) {
					free_set(set);
					return new_eval_error_expr(
							ERR_TYPE_VAR, expr.box->car.box->car);
				}
			} else {
				free_set(set);
				return new_syntax_error(args[0]);
			}
			InternId symbol_id = expr.box->car.box->car.symbol_id;
			if (!add_to_set(set, symbol_id)) {
				free_set(set);
				return attach_code(
						new_eval_error_symbol(ERR_DUP_PARAM, symbol_id),
						args[0]);
			}
			expr = expr.box->cdr;
		}
		free_set(set);
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
	case S_INTEGER_TO_CHAR:
	case S_NUMBER_TO_STRING:
		for (size_t i = 0; i < n; i++) {
			CHECK_TYPE(E_NUMBER, i);
		}
		break;
	case S_DIV:
	case S_REMAINDER:
	case S_MODULO:
		for (size_t i = 0; i < n; i++) {
			CHECK_TYPE(E_NUMBER, i);
			if (i > 0 && args[i].number == 0) {
				// This is not technically a type error, but this is the
				// earliest and most convenient place to catch it.
				return new_eval_error(ERR_DIV_ZERO);
			}
		}
		break;
	case S_CAR:
	case S_CDR:
	case S_SET_CAR:
	case S_SET_CDR:
		CHECK_TYPE(E_PAIR, 0);
		break;
	case S_STRING_LENGTH:
	case S_STRING_EQ:
	case S_STRING_APPEND:
	case S_STRING_TO_SYMBOL:
	case S_STRING_TO_NUMBER:
		for (size_t i = 0; i < n; i++) {
			CHECK_TYPE(E_STRING, i);
		}
		break;
	case S_MAKE_STRING:
		CHECK_TYPE(E_NUMBER, 0);
		CHECK_TYPE(E_CHARACTER, 1);
		if (args[0].number < 0) {
			return new_eval_error_expr(ERR_NEGATIVE_SIZE, args[0]);
		}
		break;
	case S_STRING_REF:
		CHECK_TYPE(E_STRING, 0);
		CHECK_TYPE(E_NUMBER, 1);
		CHECK_RANGE(0, 1);
		break;
	case S_STRING_SET:
		CHECK_TYPE(E_STRING, 0);
		CHECK_TYPE(E_NUMBER, 1);
		CHECK_TYPE(E_CHARACTER, 2);
		CHECK_RANGE(0, 1);
		break;
	case S_SUBSTRING:
		CHECK_TYPE(E_STRING, 0);
		CHECK_TYPE(E_NUMBER, 1);
		CHECK_TYPE(E_NUMBER, 2);
		CHECK_RANGE(0, 1);
		CHECK_RANGE(0, 2);
		break;
	case S_CHAR_TO_INTEGER:
		CHECK_TYPE(E_CHARACTER, 0);
		break;
	case S_SYMBOL_TO_STRING:
		CHECK_TYPE(E_SYMBOL, 0);
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
	case E_STDPROCMACRO:
	case E_STDPROCEDURE:
		return check_stdproc(expr.stdproc, args, n);
	case E_MACRO:
	case E_PROCEDURE:
		return NULL;
	default:
		assert(false);
		return NULL;
	}
}
