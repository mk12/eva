// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "impl.h"

#include <stdbool.h>
#include <stddef.h>

// An Implementation is a function that implements a standard procedure.
typedef struct Expression (*Implementation)(struct Expression *args, size_t n);

static struct Expression macro(struct Expression *args, size_t n) {
	(void)n;
	return new_macro(retain_expression(args[0]));
}

static struct Expression eq(struct Expression *args, size_t n) {
	(void)n;
	return new_boolean(expression_eq(args[0], args[1]));
}

static struct Expression num_eq(struct Expression *args, size_t n) {
	if (n <= 1) {
		return new_boolean(true);
	}
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number == args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression num_lt(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number < args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression num_gt(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number > args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression num_le(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number <= args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression num_ge(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number >= args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression add(struct Expression *args, size_t n) {
	Number result = 0;
	for (size_t i = 0; i < n; i++) {
		result += args[i].number;
	}
	return new_number(result);
}

static struct Expression sub(struct Expression *args, size_t n) {
	if (n == 1) {
		return new_number(-args[0].number);
	}
	Number result = args[0].number;
	for (size_t i = 1; i < n; i++) {
		result -= args[i].number;
	}
	return new_number(result);
}

static struct Expression mul(struct Expression *args, size_t n) {
	Number result = 0;
	for (size_t i = 0; i < n; i++) {
		result *= args[i].number;
	}
	return new_number(result);
}

static struct Expression div(struct Expression *args, size_t n) {
	if (n == 1) {
		return new_number(1 / args[0].number);
	}
	Number result = args[0].number;
	for (size_t i = 1; i < n; i++) {
		result /= args[i].number;
	}
	return new_number(result);
}

static struct Expression rem(struct Expression *args, size_t n) {
	(void)n;
	return new_number(args[0].number % args[1].number);
}

static struct Expression mod(struct Expression *args, size_t n) {
	(void)n;
	Number a = args[0].number;
	Number m = args[1].number;
	return new_number((a % m + m) % m);
}

static struct Expression expt(struct Expression *args, size_t n) {
	(void)n;
	Number base = args[0].number;
	Number expt = args[1].number;
	if (expt < 0) {
		return new_number(0);
	}
	Number result = base;
	while (expt != 0) {
		if ((expt & 1) == 1) {
			result *= base;
		}
		expt >>= 1;
		base *= base;
	}
	return new_number(result);
}

static struct Expression not(struct Expression *args, size_t n) {
	(void)n;
	return new_boolean(!args[0].boolean);
}

static struct Expression cons(struct Expression *args, size_t n) {
	(void)n;
	return new_pair(retain_expression(args[0]), retain_expression(args[1]));
}

static struct Expression car(struct Expression *args, size_t n) {
	(void)n;
	return retain_expression(args[0].box->car);
}

static struct Expression cdr(struct Expression *args, size_t n) {
	(void)n;
	return retain_expression(args[0].box->cdr);
}

static struct Expression set_car(struct Expression *args, size_t n) {
	(void)n;
	release_expression(args[0].box->car);
	args[0].box->car = retain_expression(args[1]);
	return retain_expression(args[0]);
}

static struct Expression set_cdr(struct Expression *args, size_t n) {
	(void)n;
	release_expression(args[0].box->cdr);
	args[0].box->cdr = retain_expression(args[1]);
	return retain_expression(args[0]);
}

static struct Expression write(struct Expression *args, size_t n) {
	(void)n;
	print_expression(args[0], stdout);
	putchar('\n');
	return new_boolean(true);
}

// A mapping from standard procedures to their implementations.
static const Implementation implementation_table[N_STANDARD_PROCEDURES] = {
	[S_EVAL]       = NULL,
	[S_APPLY]      = NULL,
	[S_MACRO]      = macro,
	[S_NULLP]      = NULL,
	[S_SYMBOLP]    = NULL,
	[S_NUMBERP]    = NULL,
	[S_BOOLEANP]   = NULL,
	[S_PAIRP]      = NULL,
	[S_MACROP]     = NULL,
	[S_PROCEDUREP] = NULL,
	[S_EQ]         = eq,
	[S_NUM_EQ]     = num_eq,
	[S_NUM_LT]     = num_lt,
	[S_NUM_GT]     = num_gt,
	[S_NUM_LE]     = num_le,
	[S_NUM_GE]     = num_ge,
	[S_ADD]        = add,
	[S_SUB]        = sub,
	[S_MUL]        = mul,
	[S_DIV]        = div,
	[S_REM]        = rem,
	[S_MOD]        = mod,
	[S_EXPT]       = expt,
	[S_NOT]        = not,
	[S_CONS]       = cons,
	[S_CAR]        = car,
	[S_CDR]        = cdr,
	[S_SET_CAR]    = set_car,
	[S_SET_CDR]    = set_cdr,
	[S_READ]       = NULL,
	[S_WRITE]      = write
};

// A mapping from expression types to the type predicates they satisfy.
static const enum StandardProcedure predicate_table[N_EXPRESSION_TYPES] = {
	[E_NULL]         = S_NULLP,
	[E_SYMBOL]       = S_SYMBOLP,
	[E_NUMBER]       = S_NUMBERP,
	[E_BOOLEAN]      = S_BOOLEANP,
	[E_STDMACRO]     = S_MACROP,
	[E_STDPROCEDURE] = S_PROCEDUREP,
	[E_PAIR]         = S_PAIRP,
	[E_MACRO]        = S_MACROP,
	[E_PROCEDURE]    = S_PROCEDUREP
};

struct Expression invoke_implementation(
		enum StandardProcedure stdproc, struct Expression *args, size_t n) {
	// Handle predicates as a special case.
	if (stdproc >= S_NULLP && stdproc <= S_PROCEDUREP) {
		return new_boolean(predicate_table[args[0].type] == stdproc);
	}
	// Look up the implementation in the table.
	return implementation_table[stdproc](args, n);
}
