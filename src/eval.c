// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"
#include "error.h"
#include "impl.h"
#include "list.h"
#include "repl.h"
#include "type.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Function prototypes.
static struct EvalResult eval(
		struct Expression expr, struct Environment *env, bool allow_define);
static struct EvalResult apply(
		struct Expression expr,
		struct Expression *args,
		size_t n,
		struct Environment *env);

struct EvalResult eval_top(struct Expression expr, struct Environment *env) {
	return eval(expr, env, true);
}

// Applies a standard macro to 'args' (an array of 'n' arguments). Assumes the
// application has already been type-checked. On success, returns the resulting
// expression. Otherwise, allocates and returns an evaluation error.
static struct EvalResult apply_stdmacro(
		enum StandardMacro stdmacro,
		struct Expression *args,
		size_t n,
		struct Environment *env) {
	struct EvalResult result;
	result.err = NULL;

	// ...

	return result;
}

// Applies a standard procedure to 'args' (an array of 'n' arguments). Assumes
// the application has already been type-checked. On success, returns the
// resulting expression. Otherwise, allocates and returns an evauation error.
static struct EvalResult apply_stdprocedure(
		enum StandardProcedure stdproc,
		struct Expression *args,
		size_t n,
		struct Environment *env) {
	struct EvalResult result;
	result.err = NULL;

	switch (stdproc) {
	case S_EVAL:
		result = eval(args[0], env, false);
		break;
	case S_APPLY:;
		struct ArrayResult more = list_to_array(args[n-1], false);
		// TOOD: args[1], args[2] ... args[n-2] come before
		assert(more.exprs != NULL);
		result = apply(args[0], more.exprs, more.size, env);
		break;
	case S_READ:;
		struct ParseError *parse_err = read_sexpr(&result.expr);
		if (parse_err) {
			result.err = new_eval_error(ERR_READ);
			result.err->parse_err = parse_err;
		}
		break;
	default:
		result.expr = invoke_implementation(stdproc, args, n);
		break;
	}
	return result;
}

// Applies 'expr' to 'args' (an array of 'n' arguments). On success, returns the
// resulting expression. If type-checking or evaluation fails, allocates and
// returns an evaluation error.
static struct EvalResult apply(
		struct Expression expr,
		struct Expression *args,
		size_t n,
		struct Environment *env) {
	struct EvalResult result;
	result.err = type_check(expr, args, n);
	if (result.err) {
		return result;
	}

	switch (expr.type) {
	case E_STDMACRO:
		result = apply_stdmacro(expr.stdmacro, args, n, env);
		break;
	case E_STDPROCMACRO:
	case E_STDPROCEDURE:
		result = apply_stdprocedure(expr.stdproc, args, n, env);
		break;
	case E_MACRO:
	case E_PROCEDURE:;
		Arity arity = expr.box->arity;
		struct Environment *aug =
				new_environment(expr.box->env, (size_t)abs(arity));
		size_t limit = arity < 0 ? (size_t)ATLEAST(arity) : (size_t)arity;
		for (size_t i = 0; i < limit; i++) {
			bind(aug, expr.box->params[i], args[i]);
		}
		if (arity < 0) {
			struct Expression list = new_null();
			for (size_t i = n; i-- > limit;) {
				list = new_pair(retain_expression(args[i]), list);
			}
			bind(aug, expr.box->params[limit], list);
			release_expression(list);
		}
		result = eval(expr.box->body, aug, false);
		release_environment(aug);
		break;
	default:
		assert(false);
		break;
	}
	return result;
}

// Evaluates the 'n' expressions of 'args' in 'env', replacing each element of
// the array with its evaluated result. Upon encountering an error, releases all
// evaluation results created so far and returns the evaluation error.
static struct EvalError *eval_in_place(
		struct Expression *args, size_t n, struct Environment *env) {
	for (size_t i = 0; i < n; i++) {
		struct EvalResult result = eval(args[i], env, false);
		if (result.err) {
			for (size_t j = 0; j < i; j++) {
				release_expression(args[i]);
			}
			return result.err;
		}
		args[i] = result.expr;
	}
	return NULL;
}

// Releases the expressions created by 'eval_in_place'.
static void release_in_place(struct Expression *args, size_t n) {
	for (size_t i = 0; i < n; i++) {
		release_expression(args[i]);
	}
}

// Evaluates the application of 'expr' to 'args' (both passed unevaluated).
static struct EvalResult eval_application(
		struct Expression expr,
		struct Expression *args,
		size_t n,
		struct Environment *env,
		bool allow_define) {
	struct EvalResult result;
	result.err = NULL;

	struct EvalResult operator = eval(expr, env, false);
	if (operator.err) {
		result.err = operator.err;
		return result;
	}
	Arity arity;
	if (!expression_arity(&arity, operator.expr)) {
		result.err = new_eval_error_expr(ERR_TYPE_OPERATOR, operator.expr);
		return result;
	}
	if (!arity_allows(arity, n)) {
		result.err = new_arity_error(arity, n);
		return result;
	}

	switch (operator.expr.type) {
	case E_STDMACRO:
		if (!allow_define && operator.expr.stdmacro == F_DEFINE) {
			result.err = new_eval_error(ERR_DEFINE);
			break;
		}
		// fall through
	case E_STDPROCMACRO:
	case E_MACRO:
		result = apply(operator.expr, args, n, env);
		if (result.err) {
			break;
		}
		struct Expression transformed = result.expr;
		result = eval(transformed, env, allow_define);
		release_expression(transformed);
		break;
	case E_STDPROCEDURE:
	case E_PROCEDURE:
		result.err = eval_in_place(args, n, env);
		if (result.err) {
			break;
		}
		result = apply(operator.expr, args, n, env);
		release_in_place(args, n);
		break;
	default:
		assert(false);
		break;
	}
	return result;
}

// Evaluates a single expression in the given environment. Top-level definitions
// are only allowed if 'allow_define' is true; otherwise, they cause an error.
static struct EvalResult eval(
		struct Expression expr, struct Environment *env, bool allow_define) {
	struct EvalResult result;
	result.err = NULL;

	switch (expr.type) {
	case E_SYMBOL:;
		// Look up the variable in the environment.
		struct LookupResult look = lookup(env, expr.symbol_id);
		if (look.found) {
			result.expr = retain_expression(look.expr);
		} else {
			result.err = new_eval_error_symbol(ERR_UNBOUND_VAR, expr.symbol_id);
		}
		break;
	case E_PAIR:;
		// Get the arguments.
		struct ArrayResult args = list_to_array(expr.box->cdr, false);
		if (args.improper) {
			result.err = new_syntax_error(expr);
			break;
		}
		// Evaluate the application.
		result = eval_application(
				expr.box->car, args.exprs, args.size, env, allow_define);
		free(args.exprs);
		break;
	default:
		// Everything else is self-evaluating.
		result.expr = retain_expression(expr);
		break;
	}
	if (result.err && !result.err->has_code) {
		// Attach code to the error for context.
		attach_code(result.err, expr);
	}
	return result;
}
