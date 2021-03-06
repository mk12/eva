// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"
#include "error.h"
#include "list.h"
#include "macro.h"
#include "prelude.h"
#include "proc.h"
#include "repl.h"
#include "type.h"
#include "util.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function prototypes.
static struct EvalResult apply(
		struct Expression expr,
		struct Expression *args,
		size_t n,
		struct Environment *env);

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
	switch (stdmacro) {
	case F_QUASIQUOTE:
		return quasiquote(args[0], env);
	case F_UNQUOTE:
	case F_UNQUOTE_SPLICING:
		return (struct EvalResult){ .err = new_eval_error(ERR_UNQUOTE) };
	default:
		return invoke_stdmacro(stdmacro, args, n, env);
	}
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
	case S_LOAD:
		result.expr = new_void();
		const size_t len = strlen(PRELUDE_FILENAME);
		if (args[0].box->len == len
				&& strncmp(args[0].box->str, PRELUDE_FILENAME, len) == 0) {
			execute(PRELUDE_FILENAME, prelude_source, env, false);
			break;
		}
		char* filename = null_terminated_string(args[0]);
		char* contents = read_file(filename);
		if (contents == NULL) {
			result.err = new_eval_error_expr(ERR_LOAD, args[0]);
		} else {
			execute(filename, contents, env, false);
			free(contents);
		}
		free(filename);
		break;
	default:
		result.expr = invoke_stdprocedure(stdproc, args, n);
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
// F_LAMBDA, F_LET_*, or F_COND and its body contains more than one expression,
// wraps the expresions in an F_BEGIN block. If the operator is F_DEFINE and has
// only one argument, rewrites it to assign a void value. If the operator is
// F_DEFINE with the function definition syntax, rewrites it to use F_LAMBDA.
static void rewrite_arguments(
		struct Expression code,
		struct Expression operator,
		struct Array *args) {
	if (operator.type != E_STDMACRO) {
		return;
	}
	switch (operator.stdmacro) {
	case F_DEFINE:
		if (args->size == 1) {
			args->exprs = realloc(args->exprs, 2 * sizeof *args->exprs);
			args->exprs[1] = new_void();
			args->size = 2;
		} else if (args->size >= 2 && args->exprs[0].type == E_PAIR) {
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
		if (args->size > 2) {
			struct Expression block = new_pair(
					new_stdmacro(F_BEGIN),
					code.box->cdr.box->cdr);
			code.box->cdr.box->cdr = new_pair(block, new_null());
			args->size = 2;
			args->exprs[1] = block;
		}
		break;
	case F_COND:
		for (size_t i = 0; i < args->size; i++) {
			if (args->exprs[i].type == E_PAIR
					&& args->exprs[i].box->cdr.type == E_PAIR
					&& args->exprs[i].box->cdr.box->cdr.type == E_PAIR) {
				struct Expression block = new_pair(
						new_stdmacro(F_BEGIN),
						args->exprs[i].box->cdr);
				args->exprs[i].box->cdr = new_pair(block, new_null());
			}
		}
		break;
	default:
		break;
	}
}

struct EvalResult eval(
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
