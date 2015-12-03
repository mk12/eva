// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "eval.h"

#include "env.h"

#include <assert.h>
#include <stdlib.h>

// Evaluation error messages.
static const char *err_op_not_proc = "operator is not a procedure";
static const char *err_arity = "wrong number of arguments passed to procedure";
static const char *err_not_num = "expected operand to be a number";
static const char *err_not_bool = "expected operand to be a boolean";
static const char *err_not_proc = "expected operand to be a procedure";
static const char *err_not_pair = "expected operand to be a pair";
static const char *err_unbound_var = "use of unbound variable";
static const char *err_ill_list = "ill-formed list";
static const char *err_ill_define = "ill-formed special form: define";
static const char *err_ill_placed_define = "ill-placed special form: define";
static const char *err_ill_quote = "ill-formed special form: quote";
static const char *err_special_var = "special form can't be used as variable";
static const char *err_ill_if = "ill-formed special form: if";

#define N_SPECIAL_FORMS 9

enum SpecialForms {
	F_DEFINE,
	F_LAMBDA,
	F_QUOTE,
	F_COND,
	F_IF,
	F_LET,
	F_LET_STAR,
	F_AND,
	F_OR
};

static struct EvalResult eval(
		struct Expression expr, struct Environment *env, bool allow_define);

// Names of special forms.
static const char *special_form_names[N_SPECIAL_FORMS] = {
	"define", "lambda", "quote", "cond", "if", "let", "let*", "and", "or"
};

// Intern identifiers of special forms.
static InternID special_form_ids[N_SPECIAL_FORMS];

void setup_eval(void) {
	for (int i = 0; i < N_SPECIAL_FORMS; i++) {
		special_form_ids[i] = intern_string(special_form_names[i]);
	}
}

// Returns NULL if the procedure accepts n arguments. Returns an error message
// otherwise. Assumes proc is a procedure.
static const char *check_arg_count(struct Expression proc, int n) {
	int arity;
	if (proc.type == E_SPECIAL) {
		arity = special_arity(proc.special_type);
	} else {
		assert(proc.type == E_LAMBDA);
		arity = proc.box->lambda.arity;
	}

	if (arity >= 0) {
		return n == arity ? NULL : err_arity;
	} else {
		return n >= -(arity + 1) ? NULL : err_arity;
	}
}

// Returns NULL if all argument are of the correct type. Returns an error
// message otherwise. Assumes the correct number of arguments are provided.
static const char *check_arg_types(
		enum SpecialType type, struct Expression *args, int n) {
	switch (type) {
	case S_APPLY:
		if (args[0].type != E_SPECIAL && args[0].type != E_LAMBDA) {
			return err_not_proc;
		}
		if (args[1].type != E_PAIR) {
			return err_not_pair;
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
	case S_DIV:
	case S_REM:
		for (int i = 0; i < n; i++) {
			if (args[i].type != E_NUMBER) {
				return err_not_num;
			}
		}
		break;
	case S_EVAL:
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

// Returns NULL if the application of proc to args is valid. Returns an error
// message otherwise (including the case where proc is not a procedure).
static const char *check_application(
		struct Expression proc, struct Expression *args, int n) {
	bool special = proc.type == E_SPECIAL;
	bool lambda = proc.type == E_LAMBDA;
	if (!special && !lambda) {
		return err_op_not_proc;
	}

	const char *err_msg = check_arg_count(proc, n);
	if (err_msg) {
		return err_msg;
	}
	if (special) {
		err_msg = check_arg_types(proc.special_type, args, n);
	}
	return err_msg;
}

struct ArrayResult {
	int size;
	struct Expression *exprs;
	const char *err_msg;
};

// Converts a list of a flat array of expressions. Returns an error messge if
// the expression is not a well-formed list. Copies elements of the list
// directly to the new array, but does not alter reference counts.
static struct ArrayResult sexpr_array(struct Expression list) {
	struct ArrayResult result;
	result.size = 0;
	result.exprs = NULL;
	result.err_msg = NULL;

	struct Expression expr = list;
	while (expr.type != E_NULL) {
		if (expr.type != E_PAIR) {
			result.err_msg = err_ill_list;
			return result;
		}
		expr = expr.box->pair.cdr;
		result.size++;
	}

	result.exprs = malloc(result.size * sizeof *result.exprs);
	expr = list;
	for (int i = 0; i < result.size; i++) {
		result.exprs[i] = expr.box->pair.car;
		expr = expr.box->pair.cdr;
	}
	return result;
}

// Returns the expression resulting from applying a special procedure to
// arguments. Assumes the arguments are correct in number and in types. Provides
// an error message if the application fails (only possible for eval and apply).
static struct EvalResult apply_special(
		enum SpecialType type,
		struct Expression *args,
		int n,
		struct Environment *env) {
	struct EvalResult result;
	result.err_msg = NULL;
	switch (type) {
	case S_EVAL:
		result = eval(args[0], env, true);
		break;
	case S_APPLY:;
		struct Expression quote = new_symbol(special_form_ids[F_QUOTE]);
		struct Expression proc = new_pair(quote, new_pair(args[0], new_null()));
		struct Expression app = new_pair(proc, args[1]);
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
				result.expr = new_boolean(args[0].symbol_id == args[1].symbol_id);
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
		result.expr = new_pair(args[0], args[1]);
		break;
	case S_CAR:
		result.expr = args[0].box->pair.car;
		break;
	case S_CDR:
		result.expr = args[0].box->pair.cdr;
		break;
	case S_ADD:;
		int sum = 0;
		for (int i = 0; i < n; i++) {
			sum += args[i].number;
		}
		result.expr = new_number(sum);
		break;
	case S_SUB:
		if (n == 1) {
			result.expr = new_number(-args[0].number);
			break;
		}
		int diff = args[0].number;
		for (int i = 1; i < n; i++) {
			diff -= args[i].number;
		}
		result.expr = new_number(diff);
		break;
	case S_MUL:;
		int prod = 1;
		for (int i = 0; i < n; i++) {
			prod *= args[i].number;
		}
		result.expr = new_number(prod);
		break;
	case S_DIV:
		if (n == 1) {
			result.expr = new_number(1 / args[0].number);
			break;
		}
		int quot = args[0].number;
		for (int i = 1; i < n; i++) {
			quot /= args[i].number;
		}
		result.expr = new_number(quot);
		break;
	case S_REM:;
		int rem = args[0].number % args[1].number;
		result.expr = new_number(rem);
		break;
	case S_NOT:
		result.expr = new_boolean(!args[0].boolean);
		break;
	}
	return result;
}

// Applies a procedure to arguments. Assumes proc is a procedure.
static struct EvalResult apply(
		struct Expression proc,
		struct Expression *args,
		int n,
		struct Environment *env) {
	struct EvalResult result;
	result.err_msg = check_application(proc, args, n);
	if (result.err_msg) {
		return result;
	}

	if (proc.type == E_SPECIAL) {
		result = apply_special(proc.special_type, args, n, env);
	} else {
		assert(proc.type == E_LAMBDA);
		for (int i = 0; i < n; i++) {
			bind(env, proc.box->lambda.params[i], args[i]);
		}
		result = eval(proc.box->lambda.body, env, false);
		for (int i = n - 1; i >= 0; i--) {
			unbind_last(env, proc.box->lambda.params[i]);
		}
	}
	return result;
}

// Evaluates a single expression.
static struct EvalResult eval(
		struct Expression expr, struct Environment *env, bool allow_define) {
	struct EvalResult result;
	result.err_msg = NULL;

	switch (expr.type) {
	case E_PAIR:
		if (expr.box->pair.car.type == E_SYMBOL) {
			InternID id = expr.box->pair.car.symbol_id;
			if (id == special_form_ids[F_DEFINE]) {
				if (!allow_define) {
					result.err_msg = err_ill_placed_define;
					break;
				}
				if (expr.box->pair.cdr.type != E_PAIR
						|| expr.box->pair.cdr.box->pair.car.type != E_SYMBOL
						|| expr.box->pair.cdr.box->pair.cdr.type != E_PAIR
						|| expr.box->pair.cdr.box->pair.cdr.box->pair.cdr.type
						!= E_NULL) {
					result.err_msg = err_ill_define;
					break;
				}
				InternID id = expr.box->pair.cdr.box->pair.car.symbol_id;
				for (int i = 0; i < N_SPECIAL_FORMS; i++) {
					if (id == special_form_ids[i]) {
						result.err_msg = err_special_var;
						break;
					}
				}
				if (result.err_msg) {
					break;
				}
				struct EvalResult val = eval(
						expr.box->pair.cdr.box->pair.cdr.box->pair.car,
						env,
						false);
				if (val.err_msg) {
					result.err_msg = val.err_msg;
					break;
				}
				result.expr = val.expr;
				bind(env, id, val.expr);
				break;
			} else if (id == special_form_ids[F_LAMBDA]) {
				break;
			} else if (id == special_form_ids[F_QUOTE]) {
				if (expr.box->pair.cdr.type != E_PAIR
						|| expr.box->pair.cdr.box->pair.cdr.type != E_NULL) {
					result.err_msg = err_ill_quote;
					break;
				}
				result.expr = retain_expression(
						expr.box->pair.cdr.box->pair.car);
				break;
			} else if (id == special_form_ids[F_COND]) {
				break;
			} else if (id == special_form_ids[F_IF]) {
				struct ArrayResult args = sexpr_array(expr.box->pair.cdr);
				if (args.err_msg) {
					result.err_msg = args.err_msg;
					break;
				}
				if (args.size != 3) {
					result.err_msg = err_ill_if;
					break;
				}
				struct EvalResult cond = eval(args.exprs[0], env, false);
				if (cond.err_msg) {
					result.err_msg = cond.err_msg;
					break;
				}
				int i = (cond.expr.type != E_BOOLEAN || cond.expr.boolean) ? 1 : 2;
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
				break;
			} else if (id == special_form_ids[F_LET_STAR]) {
				break;
			} else if (id == special_form_ids[F_AND]) {
				struct ArrayResult args = sexpr_array(expr.box->pair.cdr);
				if (args.err_msg) {
					result.err_msg = args.err_msg;
					break;
				}
				result.expr = new_boolean(true);
				for (int i = 0; i < args.size; i++) {
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
				struct ArrayResult args = sexpr_array(expr.box->pair.cdr);
				if (args.err_msg) {
					result.err_msg = args.err_msg;
					break;
				}
				result.expr = new_boolean(false);
				for (int i = 0; i < args.size; i++) {
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
		struct EvalResult proc = eval(expr.box->pair.car, env, false);
		if (proc.err_msg) {
			result.err_msg = proc.err_msg;
			break;
		}
		struct ArrayResult args = sexpr_array(expr.box->pair.cdr);
		if (args.err_msg) {
			result.err_msg = args.err_msg;
			break;
		}
		for (int i = 0; i < args.size; i++) {
			struct EvalResult arg = eval(args.exprs[i], env, false);
			if (arg.err_msg) {
				result.err_msg = arg.err_msg;
				break;
			}
			args.exprs[i] = arg.expr;
		}
		if (!result.err_msg) {
			result = apply(proc.expr, args.exprs, args.size, env);
		}
		release_expression(proc.expr);
		for (int i = 0; i < args.size; i++) {
			release_expression(args.exprs[i]);
		}
		free(args.exprs);
		break;
	case E_SYMBOL:
		for (int i = 0; i < N_SPECIAL_FORMS; i++) {
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

struct EvalResult eval_top(struct Expression expr, struct Environment *env) {
	return eval(expr, env, true);
}
