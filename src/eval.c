// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"
#include "error.h"
#include "repl.h"
#include "type.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Special forms are the core of the Eva language. They are the syntactical
// forms that require special evaluation rules.
#define N_SPECIAL_FORMS 10
enum SpecialForms {
	F_AND,
	F_BEGIN,
	F_COND,
	F_DEFINE,
	F_IF,
	F_LAMBDA,
	F_LET,
	F_LET_STAR,
	F_OR,
	F_QUOTE
};

// Names of special forms.
static const char *special_form_names[N_SPECIAL_FORMS] = {
	[F_AND]      = "and",
	[F_BEGIN]    = "begin",
	[F_COND]     = "cond",
	[F_DEFINE]   = "define",
	[F_IF]       = "if",
	[F_LAMBDA]   = "lambda",
	[F_LET]      = "let",
	[F_LET_STAR] = "let*",
	[F_OR]       = "or",
	[F_QUOTE]    = "quote"
};

// Intern identifiers of special form names.
static InternId special_form_ids[N_SPECIAL_FORMS];

// Function prototypes.
static struct EvalResult eval(
		struct Expression expr, struct Environment *env, bool allow_define);

void setup_eval(void) {
	// Intern all the special form names.
	for (size_t i = 0; i < N_SPECIAL_FORMS; i++) {
		special_form_ids[i] = intern_string(special_form_names[i]);
	}
}

// Applies a special procedure to 'args' (an array of 'n' arguments). Assumes
// the application has already been type-checked. On success, returns the
// resulting expression. Otherwise, allocates and returns an evauation error
// (can only happen with S_EVAL and S_APPLY).
static struct EvalResult apply_special(
		enum SpecialType type,
		struct Expression *args,
		size_t n,
		struct Environment *env) {
	struct EvalResult result;
	result.err = NULL;
	switch (type) {
	case S_EVAL:
		result = eval(args[0], env, false);
		break;
	case S_APPLY:;
		struct Expression app = new_pair(
				new_pair(
					new_symbol(special_form_ids[F_QUOTE]),
					new_pair(retain_expression(args[0]), new_null())),
				retain_expression(args[1]));
		result = eval(app, env, false);
		release_expression(app);
		break;
	case S_NULL:
		result.expr = new_boolean(args[0].type == E_NULL);
		break;
	case S_SYMBOL:
		result.expr = new_boolean(args[0].type == E_SYMBOL);
		break;
	case S_NUMBER:
		result.expr = new_boolean(args[0].type == E_NUMBER);
		break;
	case S_BOOLEAN:
		result.expr = new_boolean(args[0].type == E_BOOLEAN);
		break;
	case S_PAIR:
		result.expr = new_boolean(args[0].type == E_PAIR);
		break;
	case S_PROCEDURE:;
		enum ExpressionType t = args[0].type;
		result.expr = new_boolean(t == E_LAMBDA || t == E_SPECIAL);
		break;
	case S_EQ:;
		if (args[0].type != args[1].type) {
			result.expr = new_boolean(false);
			break;
		}
		switch (args[0].type) {
			case E_NULL:
				result.expr = new_boolean(true);
				break;
			case E_SYMBOL:
				result.expr = new_boolean(
						args[0].symbol_id == args[1].symbol_id);
				break;
			case E_NUMBER:
				result.expr = new_boolean(args[0].number == args[1].number);
				break;
			case E_BOOLEAN:
				result.expr = new_boolean(args[0].boolean == args[1].boolean);
				break;
			case E_SPECIAL:
				result.expr = new_boolean(
						args[0].special_type == args[1].special_type);
				break;
			case E_PAIR:
			case E_LAMBDA:
				result.expr = new_boolean(args[0].box == args[1].box);
				break;
		}
	case S_NUM_EQ:
		result.expr = new_boolean(args[0].number == args[1].number);
		break;
	case S_LT:
		result.expr = new_boolean(args[0].number < args[1].number);
		break;
	case S_GT:
		result.expr = new_boolean(args[0].number > args[1].number);
		break;
	case S_LE:
		result.expr = new_boolean(args[0].number <= args[1].number);
		break;
	case S_GE:
		result.expr = new_boolean(args[0].number >= args[1].number);
		break;
	case S_CONS:
		result.expr = new_pair(
				retain_expression(args[0]),
				retain_expression(args[1]));
		break;
	case S_CAR:
		result.expr = retain_expression(args[0].box->car);
		break;
	case S_CDR:
		result.expr = retain_expression(args[0].box->cdr);
		break;
	case S_ADD:;
		long sum = 0;
		for (size_t i = 0; i < n; i++) {
			sum += args[i].number;
		}
		result.expr = new_number(sum);
		break;
	case S_SUB:
		if (n == 1) {
			result.expr = new_number(-args[0].number);
			break;
		}
		long diff = args[0].number;
		for (size_t i = 1; i < n; i++) {
			diff -= args[i].number;
		}
		result.expr = new_number(diff);
		break;
	case S_MUL:;
		long prod = 1;
		for (size_t i = 0; i < n; i++) {
			prod *= args[i].number;
		}
		result.expr = new_number(prod);
		break;
	case S_DIV:
		if (n == 1) {
			result.expr = new_number(1 / args[0].number);
			break;
		}
		long quot = args[0].number;
		for (size_t i = 1; i < n; i++) {
			quot /= args[i].number;
		}
		result.expr = new_number(quot);
		break;
	case S_REM:;
		long rem = args[0].number % args[1].number;
		result.expr = new_number(rem);
		break;
	case S_NOT:
		result.expr = new_boolean(!args[0].boolean);
		break;
	case S_READ:;
		struct ParseError *parse_err = read_sexpr(&result.expr);
		if (err) {
			result.err = new_eval_error(ERR_READ);
			result.err->parse_err = parse_err;
		}
		break;
	case S_WRITE:
		print_expression(args[0], stdout);
		putchar('\n');
		result.expr = new_boolean(true);
		break;
	}
	return result;
}

// Applies 'proc' to 'args' (an array of 'n' arguments). On success, returns the
// resulting expression. If type-checking or evaluation fails, allocates and
// returns an evaluation error.
static struct EvalResult apply(
		struct Expression proc,
		struct Expression *args,
		size_t n,
		struct Environment *env) {
	struct EvalResult result;
	// Check the number of arguments and their types.
	result.err = type_check(proc, args, n);
	if (result.err) {
		return result;
	}

	if (proc.type == E_SPECIAL) {
		result = apply_special(proc.special_type, args, n, env);
	} else {
		assert(proc.type == E_LAMBDA);
		int arity = proc.box->arity;
		size_t limit = arity < 0 ? (size_t)(-(arity + 1)) : n;
		size_t n_passed = arity < 0 ? limit + 1 : limit;
		// Bind each argument to its corresponding formal parameter.
		for (size_t i = 0; i < limit; i++) {
			bind(env, proc.box->params[i], args[i]);
		}
		// If a variable number of arguments is allowed, collect them in a list
		// and pass it as the final parameter.
		if (arity < 0) {
			struct Expression list = new_null();
			for (size_t i = 1; i <= n - limit; i++) {
				list = new_pair(retain_expression(args[n-i]), list);
			}
			bind(env, proc.box->params[limit], list);
			release_expression(list);
		}
		// Evaluate the body of the procedure.
		result = eval(proc.box->body, env, false);
		// Unbind all the arguments.
		for (size_t i = 1; i <= n_passed; i++) {
			unbind_last(env, proc.box->params[n_passed-i]);
		}
	}
	return result;
}

struct EvalResult eval_top(struct Expression expr, struct Environment *env) {
	return eval(expr, env, true);
}

// Evaluates a single expression.
static struct EvalResult eval(
		struct Expression expr, struct Environment *env, bool allow_define) {
	struct EvalResult result;
	result.err = NULL;

	switch (expr.type) {
	case E_PAIR:
		if (expr.box->car.type == E_SYMBOL) {
			InternId id = expr.box->car.symbol_id;
			if (id == special_form_ids[F_DEFINE]) {
				if (!allow_define) {
					result.err_msg = err_ill_placed_define;
					break;
				}
				if (expr.box->cdr.type != E_PAIR
						|| expr.box->cdr.box->car.type != E_SYMBOL
						|| expr.box->cdr.box->cdr.type != E_PAIR
						|| expr.box->cdr.box->cdr.box->cdr.type
						!= E_NULL) {
					result.err_msg = err_ill_define;
					break;
				}
				InternId name_id = expr.box->cdr.box->car.symbol_id;
				for (size_t i = 0; i < N_SPECIAL_FORMS; i++) {
					if (name_id == special_form_ids[i]) {
						result.err_msg = err_special_var;
						break;
					}
				}
				if (result.err_msg) {
					break;
				}
				struct EvalResult val = eval(
					expr.box->cdr.box->cdr.box->car,
					env,
					false);
				if (val.err_msg) {
					result.err_msg = val.err_msg;
					break;
				}
				result.expr = val.expr;
				bind(env, name_id, val.expr);
				break;
			} else if (id == special_form_ids[F_LAMBDA]) {
				struct ArrayResult parts = sexpr_array(expr.box->cdr, false);
				if (parts.err_msg) {
					result.err_msg = parts.err_msg;
					break;
				}
				if (parts.size < 2) {
					result.err_msg = err_ill_lambda;
					free(parts.exprs);
					break;
				}
				struct ArrayResult params = sexpr_array(parts.exprs[0], true);
				if (params.err_msg) {
					result.err_msg = params.err_msg;
					free(parts.exprs);
					break;
				}
				if ((size_t)(int)params.size != params.size) {
					result.err_msg = err_ill_lambda;
					free(parts.exprs);
					break;
				}
				struct Expression body;
				if (parts.size == 2) {
					body = retain_expression(parts.exprs[1]);
				} else {
					body = new_pair(
						new_symbol(special_form_ids[F_BEGIN]),
						retain_expression(expr.box->cdr.box->cdr)
					);
				}
				InternId *param_ids = malloc(params.size * sizeof *param_ids);
				for (size_t i = 0; i < params.size; i++) {
					if (params.exprs[i].type != E_SYMBOL) {
						result.err_msg = err_ill_lambda;
						break;
					}
					param_ids[i] = params.exprs[i].symbol_id;
					for (size_t j = 0; j < i; j++) {
						if (param_ids[i] == param_ids[j]) {
							result.err_msg = err_dup_param;
							break;
						}
					}
					if (result.err_msg) {
						break;
					}
				}
				if (result.err_msg) {
					free(params.exprs);
					free(parts.exprs);
					release_expression(body);
					break;
				}
				free(params.exprs);
				int arity = params.dot ? (int)-params.size : (int)params.size;
				result.expr = new_lambda(arity, param_ids, body);
				free(parts.exprs);
				break;
			} else if (id == special_form_ids[F_QUOTE]) {
				if (expr.box->cdr.type != E_PAIR
						|| expr.box->cdr.box->cdr.type != E_NULL) {
					result.err_msg = err_ill_quote;
					break;
				}
				result.expr = retain_expression(
						expr.box->cdr.box->car);
				break;
			} else if (id == special_form_ids[F_BEGIN]) {
				struct ArrayResult args = sexpr_array(expr.box->cdr, false);
				if (args.err_msg) {
					result.err_msg = args.err_msg;
					break;
				}
				if (args.size < 1) {
					result.err_msg = err_ill_begin;
					free(args.exprs);
					break;
				}
				for (size_t i = 0; i < args.size - 1; i++) {
					struct EvalResult arg = eval(args.exprs[i], env, true);
					if (arg.err_msg) {
						result.err_msg = arg.err_msg;
						break;
					}
				}
				if (result.err_msg) {
					free(args.exprs);
					break;
				}
				result = eval(args.exprs[args.size-1], env, false);
				if (result.err_msg) {
					free(args.exprs);
					break;
				}
				for (size_t i = 2; i <= args.size; i++) {
					struct Expression arg = args.exprs[args.size-i];
					if (arg.type == E_PAIR
							&& arg.box->car.type == E_SYMBOL
							&& arg.box->car.symbol_id == special_form_ids[F_DEFINE]) {
						unbind_last(env, arg.box->cdr.box->car.symbol_id);
					}
				}
				free(args.exprs);
				break;
			} else if (id == special_form_ids[F_COND]) {
				struct ArrayResult parts = sexpr_array(expr.box->cdr, false);
				if (parts.err_msg) {
					result.err_msg = parts.err_msg;
					break;
				}
				if (parts.size < 1) {
					result.err_msg = err_ill_cond;
					free(parts.exprs);
					break;
				}
				size_t i;
				bool found = false;
				for (i = 0; i < parts.size; i++) {
					struct ArrayResult args = sexpr_array(parts.exprs[i], false);
					if (args.err_msg) {
						result.err_msg = args.err_msg;
						break;
					}
					if (args.size < 2) {
						result.err_msg = err_ill_cond;
						break;
					}
					struct EvalResult cond = eval(args.exprs[0], env, false);
					if (cond.err_msg) {
						result.err_msg = cond.err_msg;
						free(args.exprs);
						break;
					}
					bool yes = cond.expr.type != E_BOOLEAN || cond.expr.boolean;
					release_expression(cond.expr);
					if (yes) {
						if (args.size == 2) {
							result = eval(args.exprs[1], env, false);
						} else {
							struct Expression block = new_pair(
								new_symbol(special_form_ids[F_BEGIN]),
								retain_expression(parts.exprs[i].box->cdr)
							);
							result = eval(block, env, false);
							release_expression(block);
						}
						free(args.exprs);
						found = true;
						break;
					}
					free(args.exprs);
				}
				if (!result.err_msg && !found) {
					result.err_msg = err_non_exhaustive;
				}
				if (result.err_msg) {
					free(parts.exprs);
					break;
				}
				for (; i < parts.size; i++) {
					result.err_msg = check_list(parts.exprs[i]);
					if (result.err_msg) {
						break;
					}
				}
				free(parts.exprs);
				break;
			} else if (id == special_form_ids[F_IF]) {
				struct ArrayResult args = sexpr_array(expr.box->cdr, false);
				if (args.err_msg) {
					result.err_msg = args.err_msg;
					break;
				}
				if (args.size != 3) {
					result.err_msg = err_ill_if;
					free(args.exprs);
					break;
				}
				struct EvalResult cond = eval(args.exprs[0], env, false);
				if (cond.err_msg) {
					result.err_msg = cond.err_msg;
					free(args.exprs);
					break;
				}
				size_t i = (cond.expr.type != E_BOOLEAN || cond.expr.boolean) ? 1 : 2;
				release_expression(cond.expr);
				struct EvalResult branch = eval(args.exprs[i], env, false);
				free(args.exprs);
				if (branch.err_msg) {
					result.err_msg = branch.err_msg;
					break;
				}
				result.expr = branch.expr;
				break;
			} else if (id == special_form_ids[F_LET]) {
				struct ArrayResult parts = sexpr_array(expr.box->cdr, false);
				if (parts.err_msg) {
					result.err_msg = parts.err_msg;
					break;
				}
				if (parts.size < 2) {
					result.err_msg = err_ill_let;
					free(parts.exprs);
					break;
				}
				if (parts.exprs[0].type != E_NULL && parts.exprs[0].type != E_PAIR) {
					result.err_msg = err_ill_let;
					free(parts.exprs);
					break;
				}
				struct ArrayResult bindings = sexpr_array(parts.exprs[0], false);
				struct Expression *vals = malloc(bindings.size * sizeof *vals);
				if (bindings.err_msg) {
					result.err_msg = bindings.err_msg;
					free(parts.exprs);
					break;
				}
				size_t i;
				for (i = 0; i < bindings.size; i++) {
					struct Expression b = bindings.exprs[i];
					if (b.type != E_PAIR
							|| b.box->car.type != E_SYMBOL
							|| b.box->cdr.type != E_PAIR
							|| b.box->cdr.box->cdr.type != E_NULL) {
						result.err_msg = err_ill_let;
						break;
					}
					struct EvalResult val = eval(b.box->cdr.box->car, env, false);
					if (val.err_msg) {
						result.err_msg = val.err_msg;
						break;
					}
					vals[i] = val.expr;
				}
				if (result.err_msg) {
					for (size_t j = 0; j < i; j++) {
						release_expression(vals[j]);
					}
					free(bindings.exprs);
					free(parts.exprs);
					break;
				}
				for (i = 0; i < bindings.size; i++) {
					bind(env, bindings.exprs[i].box->car.symbol_id, vals[i]);
					release_expression(vals[i]);
				}
				struct Expression body;
				if (parts.size == 2) {
					body = retain_expression(parts.exprs[1]);
				} else {
					body = new_pair(
						new_symbol(special_form_ids[F_BEGIN]),
						retain_expression(expr.box->cdr.box->cdr)
					);
				}
				result = eval(body, env, true);
				release_expression(body);
				for (i = 1; i <= bindings.size; i++) {
					unbind_last(env, bindings.exprs[bindings.size-i].box->car.symbol_id);
				}
				free(bindings.exprs);
				free(parts.exprs);
				break;
			} else if (id == special_form_ids[F_LET_STAR]) {
				break;
			} else if (id == special_form_ids[F_AND]) {
				struct ArrayResult args = sexpr_array(expr.box->cdr, false);
				if (args.err_msg) {
					result.err_msg = args.err_msg;
					break;
				}
				result.expr = new_boolean(true);
				for (size_t i = 0; i < args.size; i++) {
					struct EvalResult arg = eval(args.exprs[i], env, false);
					if (arg.err_msg) {
						result.err_msg = arg.err_msg;
						break;
					}
					release_expression(result.expr);
					result.expr = arg.expr;
					if (result.expr.type == E_BOOLEAN && !result.expr.boolean) {
						break;
					}
				}
				free(args.exprs);
				break;
			} else if (id == special_form_ids[F_OR]) {
				struct ArrayResult args = sexpr_array(expr.box->cdr, false);
				if (args.err_msg) {
					result.err_msg = args.err_msg;
					break;
				}
				result.expr = new_boolean(false);
				for (size_t i = 0; i < args.size; i++) {
					struct EvalResult arg = eval(args.exprs[i], env, false);
					if (arg.err_msg) {
						result.err_msg = arg.err_msg;
						break;
					}
					release_expression(result.expr);
					result.expr = arg.expr;
					if (result.expr.type != E_BOOLEAN || result.expr.boolean) {
						break;
					}
				}
				free(args.exprs);
				break;
			}
		}
		struct EvalResult proc = eval(expr.box->car, env, false);
		if (proc.err_msg) {
			result.err_msg = proc.err_msg;
			break;
		}
		struct ArrayResult args = sexpr_array(expr.box->cdr, false);
		if (args.err_msg) {
			result.err_msg = args.err_msg;
			break;
		}
		size_t err_i = 0;
		for (size_t i = 0; i < args.size; i++) {
			struct EvalResult arg = eval(args.exprs[i], env, false);
			if (arg.err_msg) {
				err_i = i;
				result.err_msg = arg.err_msg;
				break;
			}
			args.exprs[i] = arg.expr;
		}
		if (result.err_msg) {
			release_expression(proc.expr);
			for (size_t i = 0; i < err_i; i++) {
				release_expression(args.exprs[i]);
			}
			free(args.exprs);
			break;
		}
		result = apply(proc.expr, args.exprs, args.size, env);
		release_expression(proc.expr);
		for (size_t i = 0; i < args.size; i++) {
			release_expression(args.exprs[i]);
		}
		free(args.exprs);
		break;
	case E_SYMBOL:
		for (size_t i = 0; i < N_SPECIAL_FORMS; i++) {
			if (expr.symbol_id == special_form_ids[i]) {
				result.err_msg = err_special_var;
				break;
			}
		}
		if (result.err_msg) {
			break;
		}
		struct LookupResult look = lookup(env, expr.symbol_id);
		if (look.found) {
			result.expr = retain_expression(look.expr);
		} else {
			result.err_msg = err_unbound_var;
		}
		break;
	default:
		// Everything else is self-evaluating.
		result.expr = retain_expression(expr);
		break;
	}

	return result;
}
