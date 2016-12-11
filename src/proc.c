// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "proc.h"

#include "expr.h"
#include "intern.h"
#include "parse.h"
#include "util.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// An Implementation is a function that implements a standard procedure.
typedef struct Expression (*Implementation)(struct Expression *args, size_t n);

static struct Expression s_macro(struct Expression *args, size_t n) {
	(void)n;
	return new_macro(retain_expression(args[0]));
}

static struct Expression s_eq(struct Expression *args, size_t n) {
	(void)n;
	return new_boolean(expression_eq(args[0], args[1]));
}

static struct Expression s_num_eq(struct Expression *args, size_t n) {
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

static struct Expression s_num_lt(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number < args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression s_num_gt(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number > args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression s_num_le(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number <= args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression s_num_ge(struct Expression *args, size_t n) {
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i-1].number >= args[i].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

static struct Expression s_add(struct Expression *args, size_t n) {
	Number result = 0;
	for (size_t i = 0; i < n; i++) {
		result += args[i].number;
	}
	return new_number(result);
}

static struct Expression s_sub(struct Expression *args, size_t n) {
	if (n == 1) {
		return new_number(-args[0].number);
	}
	Number result = args[0].number;
	for (size_t i = 1; i < n; i++) {
		result -= args[i].number;
	}
	return new_number(result);
}

static struct Expression s_mul(struct Expression *args, size_t n) {
	Number result = 1;
	for (size_t i = 0; i < n; i++) {
		result *= args[i].number;
	}
	return new_number(result);
}

static struct Expression s_div(struct Expression *args, size_t n) {
	if (n == 1) {
		return new_number(1 / args[0].number);
	}
	Number result = args[0].number;
	for (size_t i = 1; i < n; i++) {
		result /= args[i].number;
	}
	return new_number(result);
}

static struct Expression s_remainder(struct Expression *args, size_t n) {
	(void)n;
	return new_number(args[0].number % args[1].number);
}

static struct Expression s_modulo(struct Expression *args, size_t n) {
	(void)n;
	Number a = args[0].number;
	Number m = args[1].number;
	return new_number((a % m + m) % m);
}

static struct Expression s_expt(struct Expression *args, size_t n) {
	(void)n;
	Number base = args[0].number;
	Number expt = args[1].number;
	if (expt < 0) {
		return new_number(0);
	}
	Number result = 1;
	while (expt != 0) {
		if ((expt & 1) == 1) {
			result *= base;
		}
		expt >>= 1;
		base *= base;
	}
	return new_number(result);
}

static struct Expression s_not(struct Expression *args, size_t n) {
	(void)n;
	return new_boolean(!expression_truthy(args[0]));
}

static struct Expression s_cons(struct Expression *args, size_t n) {
	(void)n;
	return new_pair(retain_expression(args[0]), retain_expression(args[1]));
}

static struct Expression s_car(struct Expression *args, size_t n) {
	(void)n;
	return retain_expression(args[0].box->car);
}

static struct Expression s_cdr(struct Expression *args, size_t n) {
	(void)n;
	return retain_expression(args[0].box->cdr);
}

static struct Expression s_set_car(struct Expression *args, size_t n) {
	(void)n;
	release_expression(args[0].box->car);
	args[0].box->car = retain_expression(args[1]);
	return retain_expression(args[0]);
}

static struct Expression s_set_cdr(struct Expression *args, size_t n) {
	(void)n;
	release_expression(args[0].box->cdr);
	args[0].box->cdr = retain_expression(args[1]);
	return retain_expression(args[0]);
}

static struct Expression s_string_length(struct Expression *args, size_t n) {
	(void)n;
	return new_number((Number)args[0].box->len);
}

static struct Expression s_string_eq(struct Expression *args, size_t n) {
	(void)n;
	size_t len0 = args[0].box->len;
	size_t len1 = args[1].box->len;
	bool result = args[0].box == args[1].box || (len0 == len1
			&& memcmp(args[0].box->str, args[1].box->str, len0) == 0);
	return new_boolean(result);
}

static struct Expression s_substring(struct Expression *args, size_t n) {
	(void)n;
	size_t len = (size_t)(args[2].number - args[1].number);
	char *buf = xmalloc(len);
	memcpy(buf, args[0].box->str + args[1].number, len);
	return new_string(buf, len);
}

static struct Expression s_string_append(struct Expression *args, size_t n) {
	size_t len = 0;
	for (size_t i = 0; i < n; i++) {
		len += args[i].box->len;
	}
	if (len == 0) {
		return new_string(NULL, 0);
	}
	char *buf = xmalloc(len);
	char *ptr = buf;
	for (size_t i = 0; i < n; i++) {
		memcpy(ptr, args[i].box->str, args[i].box->len);
		ptr += args[i].box->len;
	}
	return new_string(buf, len);
}

static struct Expression s_char_to_integer(struct Expression *args, size_t n) {
	(void)n;
	return new_number((Number)args[0].character);
}

static struct Expression s_integer_to_char(struct Expression *args, size_t n) {
	(void)n;
	if ((Number)(char)args[0].number == args[0].number) {
		return new_character((char)args[0].number);
	}
	return new_boolean(false);
}

static struct Expression s_string_to_symbol(struct Expression *args, size_t n) {
	(void)n;
	return new_symbol(intern_string_n(args[0].box->str, args[0].box->len));
}

static struct Expression s_symbol_to_string(struct Expression *args, size_t n) {
	(void)n;
	const char* str = find_string(args[0].symbol_id);
	size_t len = strlen(str);
	char *buf = xmalloc(len);
	memcpy(buf, str, len);
	return new_string(buf, len);
}

static struct Expression s_string_to_number(struct Expression *args, size_t n) {
	(void)n;
	Number number;
	if (parse_number(args[0].box->str, args[0].box->len, &number)) {
		return new_number(number);
	}
	return new_boolean(false);
}

static struct Expression s_number_to_string(struct Expression *args, size_t n) {
	(void)n;
	size_t len = (size_t)snprintf(NULL, 0, NUMBER_FMT, args[0].number);
	char *buf = xmalloc(len);
	snprintf(buf, len, NUMBER_FMT, args[0].number);
	return new_string(buf, len);
}

static struct Expression s_write(struct Expression *args, size_t n) {
	(void)n;
	print_expression(args[0], stdout);
	putchar('\n');
	return new_void();
}

// A mapping from standard procedures to their implementations.
static const Implementation implementation_table[N_STANDARD_PROCEDURES] = {
	[S_EVAL]             = NULL,
	[S_APPLY]            = NULL,
	[S_MACRO]            = s_macro,
	[S_NULLP]            = NULL,
	[S_SYMBOLP]          = NULL,
	[S_NUMBERP]          = NULL,
	[S_BOOLEANP]         = NULL,
	[S_STRINGP]          = NULL,
	[S_PAIRP]            = NULL,
	[S_MACROP]           = NULL,
	[S_PROCEDUREP]       = NULL,
	[S_EQ]               = s_eq,
	[S_NUM_EQ]           = s_num_eq,
	[S_NUM_LT]           = s_num_lt,
	[S_NUM_GT]           = s_num_gt,
	[S_NUM_LE]           = s_num_le,
	[S_NUM_GE]           = s_num_ge,
	[S_ADD]              = s_add,
	[S_SUB]              = s_sub,
	[S_MUL]              = s_mul,
	[S_DIV]              = s_div,
	[S_REMAINDER]        = s_remainder,
	[S_MODULO]           = s_modulo,
	[S_EXPT]             = s_expt,
	[S_NOT]              = s_not,
	[S_CONS]             = s_cons,
	[S_CAR]              = s_car,
	[S_CDR]              = s_cdr,
	[S_SET_CAR]          = s_set_car,
	[S_SET_CDR]          = s_set_cdr,
	[S_STRING_LENGTH]    = s_string_length,
	[S_STRING_EQ]        = s_string_eq,
	[S_SUBSTRING]        = s_substring,
	[S_STRING_APPEND]    = s_string_append,
	[S_CHAR_TO_INTEGER]  = s_char_to_integer,
	[S_INTEGER_TO_CHAR]  = s_integer_to_char,
	[S_STRING_TO_SYMBOL] = s_string_to_symbol,
	[S_SYMBOL_TO_STRING] = s_symbol_to_string,
	[S_STRING_TO_NUMBER] = s_string_to_number,
	[S_NUMBER_TO_STRING] = s_number_to_string,
	[S_READ]             = NULL,
	[S_WRITE]            = s_write,
	[S_ERROR]            = NULL
};

// A mapping from expression types to the type predicates they satisfy.
static const enum StandardProcedure predicate_table[N_EXPRESSION_TYPES] = {
	[E_NULL]         = S_NULLP,
	[E_SYMBOL]       = S_SYMBOLP,
	[E_NUMBER]       = S_NUMBERP,
	[E_BOOLEAN]      = S_BOOLEANP,
	[E_CHARACTER]    = S_CHARP,
	[E_STDMACRO]     = S_MACROP,
	[E_STDPROCEDURE] = S_PROCEDUREP,
	[E_PAIR]         = S_PAIRP,
	[E_STRING]       = S_STRINGP,
	[E_MACRO]        = S_MACROP,
	[E_PROCEDURE]    = S_PROCEDUREP
};

struct Expression invoke_stdprocedure(
		enum StandardProcedure stdproc, struct Expression *args, size_t n) {
	// Handle predicates as a special case.
	if (stdproc >= S_NULLP && stdproc <= S_PROCEDUREP) {
		return new_boolean(predicate_table[args[0].type] == stdproc);
	}
	// Look up the implementation in the table.
	return implementation_table[stdproc](args, n);
}
