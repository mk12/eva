// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "type.h"

#include "error.h"

static struct EvalError *check_stdproc(
		enum StandardMacro stdmacro, struct Expression *args, size_t n) {
	switch (stdmacro) {
	case F_DEFINE:
	case F_SET_BANG:
		if (args[0].type != E_SYMBOL) {
			return new_type_error(E_SYMBOL, 0, args[0]);
		}
		break;
	case F_LAMBDA:
	case F_BEGIN:
	case F_QUOTE:
	case F_QUASIQUOTE:
	case F_UNQUOTE:
	case F_UNQUOTE_SPLICING:
	case F_IF:
	case F_COND:
	case F_LET:
	case F_LET_STAR:
	case F_LET_REC:
	case F_AND:
	case F_OR:
		break;
	}
}

static struct EvalError *check_stdproc(
		enum StandardProc stdproc, struct Expression *args, size_t n) {
	switch (stdproc) {
	case S_APPLY:
		if (!expression_callable(args[0])) {
			struct EvalError *err = new_eval_error(ERR_OPERAND);
			err->expr = args[0];
			return err;
		}
		// The second argument must be a well-formed list. The evaluator will
		// catch it if it's not, though, so here we only check the basic type.
		if (args[1].type != E_NULL && args[1].type != E_PAIR) {
			return new_eval_error(ERR_PROC_CALL);
		}
		break;
	case S_MACRO:
		if (args[0].type != E_STDPROCEDURE && args[0].type != E_PROCEDURE) {
			return new_type_error(E_PROCEDURE, 0, args[0]);
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
			return new_type_error(E_PAIR, 0, args[0]);
		}
		break;
	case S_NOT:
		if (args[0].type != E_BOOLEAN) {
			return new_type_error(E_BOOLEAN, 0, args[0]);
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
}
