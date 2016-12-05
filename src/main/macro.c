// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "macro.h"

#include "env.h"
#include "error.h"
#include "list.h"

#include <stdbool.h>
#include <stddef.h>

// An Implementation is a function that implements a standard macro.
typedef struct EvalResult (*Implementation)(
		struct Expression *args, size_t n, struct Environment *env);

static struct EvalResult f_define(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	struct EvalResult result = eval(args[1], env, false);
	if (!result.err) {
		bind(env, args[0].symbol_id, result.expr);
	}
	return result;
}

static struct EvalResult f_set(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	struct EvalResult result;
	result.err = NULL;

	InternId key = args[0].symbol_id;
	struct Expression *ptr = lookup(env, key);
	if (ptr) {
		result = eval(args[1], env, false);
		if (!result.err) {
			release_expression(*ptr);
			*ptr = retain_expression(result.expr);
		}
	} else {
		result.err = new_eval_error_symbol(ERR_UNBOUND_VAR, key);
	}
	return result;
}

static struct EvalResult f_lambda(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	struct Array params = list_to_array(args[0], true);
	return (struct EvalResult){
		.expr = new_procedure(
			(Arity)(params.improper ? ATLEAST(params.size - 1) : params.size),
			params.exprs,
			retain_expression(args[1]),
			retain_environment(env)),
		.err = NULL
	};
}

static struct EvalResult f_begin(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	struct EvalResult result;
	result.err = NULL;
	result.expr = new_null();
	struct Environment *aug = new_environment(env, 0);
	for (size_t i = 0; i < n; i++) {
		release_expression(result.expr);
		result = eval(args[i], aug, i != n - 1);
		if (result.err) {
			break;
		}
	}
	release_environment(aug);
	return result;
}

static struct EvalResult f_quote(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	(void)env;
	return (struct EvalResult){
		.expr = retain_expression(args[0]),
		.err = NULL
	};
}

static struct EvalResult f_if(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	struct EvalResult result = eval(args[0], env, false);
	if (!result.err) {
		size_t index = expression_truthy(result.expr) ? 1 : 2;
		result = eval(args[index], env, false);
	}
	return result;
}

static struct EvalResult f_cond(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)args;
	(void)n;
	(void)env;
	return (struct EvalResult){
		.expr = new_null(),
		.err = NULL
	};
}

static struct EvalResult f_let(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	struct EvalResult result;
	result.err = NULL;

	size_t n_bindings;
	struct Expression list = args[0];
	count_list(&n_bindings, list);
	struct Environment *aug = new_environment(env, n_bindings);
	while (list.type != E_NULL) {
		InternId id = list.box->car.box->car.symbol_id;
		struct Expression expr = list.box->car.box->cdr.box->car;
		result = eval(expr, env, false);
		if (result.err) {
			break;
		}
		bind(aug, id, result.expr);
		release_expression(result.expr);
		list = list.box->cdr;
	}
	if (!result.err) {
		result = eval(args[1], aug, false);
	}
	release_environment(aug);
	return result;
}

static struct EvalResult f_let_star(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	struct EvalResult result;
	result.err = NULL;

	size_t n_bindings;
	struct Expression list = args[0];
	count_list(&n_bindings, list);
	struct Environment *aug = new_environment(env, n_bindings);
	while (list.type != E_NULL) {
		InternId id = list.box->car.box->car.symbol_id;
		struct Expression expr = list.box->car.box->cdr.box->car;
		result = eval(expr, aug, false);
		if (result.err) {
			break;
		}
		bind(aug, id, result.expr);
		release_expression(result.expr);
		list = list.box->cdr;
	}
	if (!result.err) {
		result = eval(args[1], aug, false);
	}
	release_environment(aug);
	return result;
}

static struct EvalResult f_let_rec(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)args;
	(void)n;
	(void)env;
	return (struct EvalResult){
		.expr = new_null(),
		.err = NULL
	};
}

static struct EvalResult f_and(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	(void)env;
	struct EvalResult result;
	result.err = NULL;
	result.expr = new_boolean(true);
	for (size_t i = 0; i < n; i++) {
		release_expression(result.expr);
		result = eval(args[i], env, false);
		if (result.err || !expression_truthy(result.expr)) {
			break;
		}
	}
	return result;
}

static struct EvalResult f_or(
		struct Expression *args, size_t n, struct Environment *env) {
	(void)n;
	(void)env;
	struct EvalResult result;
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
	return result;
}

// A mapping from standard macros to their implementations.
static const Implementation implementation_table[N_STANDARD_PROCEDURES] = {
	[F_DEFINE]           = f_define,
	[F_SET]              = f_set,
	[F_LAMBDA]           = f_lambda,
	[F_BEGIN]            = f_begin,
	[F_QUOTE]            = f_quote,
	[F_QUASIQUOTE]       = NULL,
	[F_UNQUOTE]          = NULL,
	[F_UNQUOTE_SPLICING] = NULL,
	[F_IF]               = f_if,
	[F_COND]             = f_cond,
	[F_LET]              = f_let,
	[F_LET_STAR]         = f_let_star,
	[F_LET_REC]          = f_let_rec,
	[F_AND]              = f_and,
	[F_OR]               = f_or,
};

struct EvalResult invoke_stdmacro(
		enum StandardMacro stdmacro,
		struct Expression *args,
		size_t n,
		struct Environment *env) {
	// Look up the implementation in the table.
	return implementation_table[stdmacro](args, n, env);
}
