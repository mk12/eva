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

// If 'expr' (unevaluated) is the well-formed application of a standard macro to
// one operand, returns the standard macro. Otherwise, returns the integer -1.
static enum StandardMacro stdmacro_operator(
		struct Expression expr,
		struct Environment *env) {
	if (expr.type == E_PAIR) {
		struct Expression first = expr.box->car;
		if (first.type == E_SYMBOL) {
			struct Expression *ptr = lookup(env, first.symbol_id);
			if (ptr && ptr->type == E_STDMACRO) {
				first = *ptr;
			}
		}
		if (first.type == E_STDMACRO
				&& expr.box->cdr.type == E_PAIR
				&& expr.box->cdr.box->cdr.type == E_NULL) {
			return first.stdmacro;
		}
	}
	return (enum StandardMacro)-1;
}

// Returns the single operand in 'expr', which is assumed to be a well-formed
// application of a standard macro to one operand.
static struct Expression stdmacro_operand(struct Expression expr) {
	return expr.box->cdr.box->car;
}

// Applies the standard macro F_QUASIQUOTE to 'expr' (unevaluated), handling
// nested occurrences of F_UNQUOTE and F_UNQUOTE_SPLICING. Assumes the
// application has already been type-checked. On success, returns the resulting
// expression. Otherwise, allocates and returns an evaluation error.
static struct EvalResult quasiquote(
		struct Expression expr,
		struct Environment *env) {
	struct EvalResult result;
	result.err = NULL;
	if (expr.type != E_PAIR) {
		result.expr = retain_expression(expr);
		return result;
	}

	enum StandardMacro stdmacro = stdmacro_operator(expr, env);
	if (stdmacro == F_UNQUOTE) {
		result = eval(stdmacro_operand(expr), env, false);
	} else if (stdmacro == F_UNQUOTE_SPLICING) {
		result.err = new_eval_error(ERR_UNQUOTE);
	} else {
		struct Array array = list_to_array(expr, true);
		struct Expression list = new_null();
		size_t i = array.size;
		if (array.improper) {
			list = retain_expression(array.exprs[--i]);
		}
		while (i-- > 0) {
			stdmacro = stdmacro_operator(array.exprs[i], env);
			if (stdmacro == F_UNQUOTE_SPLICING) {
				result = eval(stdmacro_operand(array.exprs[i]), env, false);
				if (result.err) {
					break;
				}
				if (!concat_list(&list, result.expr, list)) {
					result.err = new_syntax_error(expr);
					break;
				}
			} else {
				result = quasiquote(array.exprs[i], env);
				if (result.err) {
					break;
				}
				list = new_pair(result.expr, list);
			}
		}
		if (result.err) {
			release_expression(list);
		} else {
			result.expr = list;
		}
		free_array(array);
	}
	return result;
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
				release_expression(*ptr);
				*ptr = retain_expression(result.expr);
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
		result = quasiquote(args[0], env);
		break;
	case F_UNQUOTE:
	case F_UNQUOTE_SPLICING:
		// We produce an error for this in 'type_check'.
		assert(false);
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
			result.err = new_read_error(parse_err);
		}
		break;
	case S_ERROR:
		result.err = new_eval_error(ERR_CUSTOM);
		result.err->array = (struct Array){
			.improper = false,
			.size = n,
			.exprs = args
		};
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
		if (arity < 0) {
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
				release_expression(args[j]);
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

// Applies rewrite rules on 'args' based on the operator. If the operator is
// F_LAMBDA of F_LET_* and its body contains more than one expression, wraps the
// expresions in an F_BEGIN block. If the operator is F_DEFINE and is using the
// function definition syntax, rewrites it to use F_LAMBDA.
static void rewrite_arguments(
		struct Expression code,
		struct Expression operator,
		struct Array *args) {
	if (operator.type != E_STDMACRO) {
		return;
	}
	switch (operator.stdmacro) {
	case F_DEFINE:
		if (args->size >= 2 && args->exprs[0].type == E_PAIR) {
			struct Expression cons = args->exprs[0];
			struct Expression name = cons.box->car;
			struct Expression list = cons.box->cdr;
			struct Expression body = code.box->cdr.box->cdr;
			code.box->cdr.box->car = name;
			code.box->cdr.box->cdr = new_pair(cons, new_null());
			cons.box->car = new_stdmacro(F_LAMBDA);
			cons.box->cdr = new_pair(list, body);
			args->size = 2;
			args->exprs[0] = name;
			args->exprs[1] = cons;
		}
		break;
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
		break;
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
		if (!(result.err && result.err->type == ERR_CUSTOM)) {
			free_array(args);
		}
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
