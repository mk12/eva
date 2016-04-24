// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"
#include "error.h"
#include "impl.h"
#include "list.h"
#include "repl.h"
#include "type.h"
#include "util.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

	switch (stdmacro) {
	case F_DEFINE:
		result = eval(args[1], env, false);
		if (!result.err) {
			bind(env, args[0].symbol_id, result.expr);
		}
		break;
	case F_SET:;
		InternId key = args[0].symbol_id;
		struct Expression *ptr = lookup(env, key);
		if (ptr) {
			result = eval(args[1], env, false);
			if (!result.err) {
				*ptr = result.expr;
			}
		} else {
			result.err = new_eval_error_symbol(ERR_UNBOUND_VAR, key);
		}
		break;
	case F_LAMBDA:;
		struct Array params = list_to_array(args[0], true);
		result.expr = new_procedure(
			(Arity)(params.improper ? ATLEAST(params.size - 1) : params.size),
			params.exprs,
			retain_expression(args[1]),
			retain_environment(env));
		break;
	case F_BEGIN:
		result.expr = new_null();
		struct Environment *aug = new_environment(env, 1);
		for (size_t i = 0; i < n; i++) {
			release_expression(result.expr);
			result = eval(args[i], aug, i != n - 1);
			if (result.err) {
				break;
			}
		}
		release_environment(aug);
		break;
	case F_QUOTE:
		result.expr = retain_expression(args[0]);
		break;
	case F_QUASIQUOTE:
	case F_UNQUOTE:
	case F_UNQUOTE_SPLICING:
		break;
	case F_IF:
		result = eval(args[0], env, false);
		if (!result.err) {
			size_t index = expression_truthy(result.expr) ? 1 : 2;
			result = eval(args[index], env, false);
		}
		break;
	case F_COND:
	case F_LET:
	case F_LET_STAR:
	case F_LET_REC:
		break;
	case F_AND:
		result.expr = new_boolean(true);
		for (size_t i = 0; i < n; i++) {
			release_expression(result.expr);
			result = eval(args[i], env, false);
			if (result.err || !expression_truthy(result.expr)) {
				break;
			}
		}
		break;
	case F_OR:
		result.expr = new_boolean(false);
		for (size_t i = 0; i < n; i++) {
			struct EvalResult res = eval(args[i], env, false);
			if (res.err || expression_truthy(res.expr)) {
				release_expression(result.expr);
				result = res;
				break;
			}
			release_expression(res.expr);
		}
		break;
	}
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
		struct Array array = list_to_array(args[n-1], false);
		assert(!array.improper);
		if (n == 2) {
			result = apply(args[0], array.exprs, array.size, env);
		} else {
			size_t before = n - 2;
			size_t total = before + array.size;
			struct Expression *all = xmalloc(total * sizeof *all);
			memcpy(all, args + 1, before * sizeof *all);
			memcpy(all + before, array.exprs, array.size * sizeof *all);
			result = apply(args[0], all, total, env);
			free(all);
		}
		free_array(array);
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
		// Don't create an environment if there are no parameters.
		if (arity == 0) {
			result = eval(expr.box->body, expr.box->env, false);
			break;
		}
		// Bind the formal parameters in a new environment.
		struct Environment *aug =
				new_environment(expr.box->env, (size_t)abs(arity));
		size_t limit = arity < 0 ? (size_t)ATLEAST(arity) : (size_t)arity;
		for (size_t i = 0; i < limit; i++) {
			bind(aug, expr.box->params[i].symbol_id, args[i]);
		}
		// Collect extra arguments in a list.
		if (n > limit) {
			assert(arity < 0);
			struct Array array = {
				.improper = false,
				.size = n - limit,
				.exprs = args + limit
			};
			struct Expression list = array_to_list(array);
			bind(aug, expr.box->params[limit].symbol_id, list);
			release_expression(list);
		}
		// Evaluate the body.
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

// Evaluates the application of 'expr' (evaluated) to 'args' (unevaluated).
static struct EvalResult eval_application(
		struct Expression expr,
		struct Expression *args,
		size_t n,
		struct Environment *env,
		bool allow_define) {
	struct EvalResult result;
	result.err = NULL;

	Arity arity;
	if (!expression_arity(&arity, expr)) {
		result.err = new_eval_error_expr(ERR_TYPE_OPERATOR, expr);
		return result;
	}
	if (!arity_allows(arity, n)) {
		result.err = new_arity_error(arity, n);
		return result;
	}

	switch (expr.type) {
	case E_STDMACRO:
		if (!allow_define && expr.stdmacro == F_DEFINE) {
			result.err = new_eval_error(ERR_DEFINE);
			break;
		}
		result = apply(expr, args, n, env);
		break;
	case E_STDPROCMACRO:
	case E_MACRO:
		result = apply(expr, args, n, env);
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
		result = apply(expr, args, n, env);
		release_in_place(args, n);
		break;
	default:
		assert(false);
		break;
	}
	return result;
}

// Applies rewrite rules on 'args' based on the operator. Specifically, if the
// operator is F_LAMBDA or F_LET_* and its body contains more than one
// expression, it wraps the expressions in a single F_BEGIN block.
static void rewrite_arguments(
		struct Expression code,
		struct Expression operator,
		struct Array *args) {
	if (operator.type != E_STDMACRO) {
		return;
	}
	switch (operator.stdmacro) {
	case F_LAMBDA:
	case F_LET:
	case F_LET_STAR:
	case F_LET_REC:
		if (args->size > 2) {
			struct Expression block = new_pair(
					new_stdmacro(F_BEGIN),
					code.box->cdr.box->cdr);
			code.box->cdr.box->cdr = new_pair(block, new_null());
			args->size = 2;
			args->exprs[1] = block;
		}
	default:
		break;
	}
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
		struct Expression *ptr = lookup(env, expr.symbol_id);
		if (ptr) {
			result.expr = retain_expression(*ptr);
		} else {
			result.err = new_eval_error_symbol(ERR_UNBOUND_VAR, expr.symbol_id);
		}
		break;
	case E_PAIR:;
		// Get the arguments.
		struct Array args = list_to_array(expr.box->cdr, false);
		if (args.improper) {
			result.err = new_syntax_error(expr);
			break;
		}
		// Evaluate the application.
		result = eval(expr.box->car, env, false);
		if (!result.err) {
			struct Expression operator = result.expr;
			rewrite_arguments(expr, operator, &args);
			result = eval_application(
					result.expr, args.exprs, args.size, env, allow_define);
			release_expression(operator);
		}
		free_array(args);
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
