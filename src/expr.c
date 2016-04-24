// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "expr.h"

#include "env.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Change to 1 to enable reference count logging.
#define REF_COUNT_LOGGING 0

// Counters for the number of allocated boxes and the sum of all refernce
// counts. Used for debugging memory bugs.
#if REF_COUNT_LOGGING
static int total_box_count = 0;
static int total_ref_count = 0;
#endif

// A pair containing the name and arity of a macro or procedure.
struct NameArity {
	const char *name;
	Arity arity;
};

// User-facing expression type names.
static const char *const expr_type_names[N_EXPRESSION_TYPES] = {
	[E_NULL]         = "NULL",
	[E_SYMBOL]       = "SYMBOL",
	[E_NUMBER]       = "NUMBER",
	[E_BOOLEAN]      = "BOOLEAN",
	[E_STDMACRO]     = "MACRO",
	[E_STDPROCMACRO] = "PROCEDURE",
	[E_STDPROCEDURE] = "PROCEDURE",
	[E_PAIR]         = "PAIR",
	[E_MACRO]        = "MACRO",
	[E_PROCEDURE]    = "PROCEDURE"
};

// Names and arities of standard macros.
static const struct NameArity stdmacro_name_arity[N_STANDARD_MACROS] = {
	[F_DEFINE]           = {"define", 2},
	[F_SET]              = {"set!", 2},
	[F_LAMBDA]           = {"lambda", ATLEAST(2)},
	[F_BEGIN]            = {"begin", ATLEAST(1)},
	[F_QUOTE]            = {"quote", 1},
	[F_QUASIQUOTE]       = {"quasiquote", 1},
	[F_UNQUOTE]          = {"unquote", 1},
	[F_UNQUOTE_SPLICING] = {"unquote-splicing", 1},
	[F_IF]               = {"if", 3},
	[F_COND]             = {"cond", ATLEAST(1)},
	[F_LET]              = {"let", ATLEAST(2)},
	[F_LET_STAR]         = {"let*", ATLEAST(2)},
	[F_LET_REC]          = {"letrec", ATLEAST(2)},
	[F_AND]              = {"and", ATLEAST(0)},
	[F_OR]               = {"or", ATLEAST(0)}
};

// Names and arities of standard procedures.
static const struct NameArity stdproc_name_arity[N_STANDARD_PROCEDURES] = {
	[S_EVAL]       = {"eval", 1},
	[S_APPLY]      = {"apply", ATLEAST(2)},
	[S_MACRO]      = {"macro", 1},
	[S_NULLP]      = {"null?", 1},
	[S_SYMBOLP]    = {"symbol?", 1},
	[S_NUMBERP]    = {"number?", 1},
	[S_BOOLEANP]   = {"boolean?", 1},
	[S_PAIRP]      = {"pair?", 1},
	[S_MACROP]     = {"macro?", 1},
	[S_PROCEDUREP] = {"procedure?", 1},
	[S_EQ]         = {"eq?", 2},
	[S_NUM_EQ]     = {"=", ATLEAST(0)},
	[S_NUM_LT]     = {"<", ATLEAST(0)},
	[S_NUM_GT]     = {">", ATLEAST(0)},
	[S_NUM_LE]     = {"<=", ATLEAST(0)},
	[S_NUM_GE]     = {">=", ATLEAST(0)},
	[S_ADD]        = {"+", ATLEAST(0)},
	[S_SUB]        = {"-", ATLEAST(1)},
	[S_MUL]        = {"*", ATLEAST(0)},
	[S_DIV]        = {"/", ATLEAST(1)},
	[S_REM]        = {"remainder", 2},
	[S_MOD]        = {"modulo", 2},
	[S_EXPT]       = {"expt", 2},
	[S_NOT]        = {"not", 1},
	[S_CONS]       = {"cons", 2},
	[S_CAR]        = {"car", 1},
	[S_CDR]        = {"cdr", 1},
	[S_SET_CAR]    = {"set-car!", 2},
	[S_SET_CDR]    = {"set-cdr!", 2},
	[S_READ]       = {"read", 0},
	[S_WRITE]      = {"write", 1}
};

const char *expression_type_name(enum ExpressionType type) {
	return expr_type_names[type];
}

struct Environment *new_standard_environment(void) {
	struct Environment *env = new_base_environment();
	// Bind standard macros.
	for (int i = 0; i < N_STANDARD_MACROS; i++) {
		InternId id = intern_string(stdmacro_name_arity[i].name);
		bind(env, id, new_stdmacro((enum StandardMacro)i));
	}
	// Bind standard procedures.
	for (int i = 0; i < N_STANDARD_PROCEDURES; i++) {
		InternId id = intern_string(stdproc_name_arity[i].name);
		bind(env, id, new_stdprocedure((enum StandardProcedure)i));
	}
	// Bind "else" to true (used in 'cond').
	bind(env, intern_string("else"), new_boolean(true));
	return env;
}

struct Expression new_null(void) {
	return (struct Expression){ .type = E_NULL };
}

struct Expression new_symbol(InternId symbol_id) {
	return (struct Expression){ .type = E_SYMBOL, .symbol_id = symbol_id };
}

struct Expression new_number(Number number) {
	return (struct Expression){ .type = E_NUMBER, .number = number };
}

struct Expression new_boolean(bool boolean) {
	return (struct Expression){ .type = E_BOOLEAN, .boolean = boolean };
}

struct Expression new_stdmacro(enum StandardMacro stdmacro) {
	return (struct Expression){ .type = E_STDMACRO, .stdmacro = stdmacro };
}

struct Expression new_stdprocedure(enum StandardProcedure stdproc) {
	return (struct Expression){ .type = E_STDPROCEDURE, .stdproc = stdproc };
}

// Logs information about reference counts to standard error.
#if REF_COUNT_LOGGING
static void log_ref_count(const char *action, struct Expression expr) {
	fprintf(stderr, "[%d/%d] %7s %6s [%d] ",
			total_ref_count,
			total_box_count,
			action,
			expression_type_name(expr.type),
			expr.box->ref_count);
	switch (expr.type) {
	case E_PAIR:
		putc('(', stderr);
		print_expression(expr.box->car, stderr);
		fputs(" . ", stderr);
		print_expression(expr.box->cdr, stderr);
		break;
	case E_MACRO:
		fputs("(macro ", stderr);
		// fall through
	case E_PROCEDURE:
		fputs("(lambda (?) ", stderr);
		print_expression(expr.box->body, stderr);
		break;
	default:
		assert(false);
		break;
	}
	if (expr.type == E_MACRO) {
		putc(')', stderr);
	}
	fputs(")\n", stderr);
}
#endif

struct Expression new_pair(struct Expression car, struct Expression cdr) {
	struct Box *box = xmalloc(sizeof *box);
	box->ref_count = 1;
	box->car = car;
	box->cdr = cdr;
	struct Expression expr = { .type = E_PAIR, .box = box };
#if REF_COUNT_LOGGING
	total_box_count++;
	total_ref_count++;
	log_ref_count("create", expr);
#endif
	return expr;
}

struct Expression new_macro(struct Expression expr) {
	switch (expr.type) {
	case E_STDPROCEDURE:
		return (struct Expression){
			.type = E_STDPROCMACRO,
			.stdproc = expr.stdproc
		};
	case E_PROCEDURE:
		return (struct Expression){
			.type = E_MACRO,
			.box = expr.box
		};
	default:
		assert(false);
		return expr;
	}
}

struct Expression new_procedure(
		Arity arity,
		InternId *params,
		struct Expression body,
		struct Environment *env) {
	struct Box *box = xmalloc(sizeof *box);
	box->ref_count = 1;
	box->arity = arity;
	box->params = params;
	box->body = body;
	box->env = env;
	struct Expression expr = { .type = E_PROCEDURE, .box = box };
#if REF_COUNT_LOGGING
	total_box_count++;
	total_ref_count++;
	log_ref_count("create", expr);
#endif
	return expr;
}

static void dealloc_expression(struct Expression expr) {
#if REF_COUNT_LOGGING
	switch (expr.type) {
	case E_PAIR:
	case E_MACRO:
	case E_PROCEDURE:
		total_box_count--;
		log_ref_count("dealloc", expr);
		break;
	default:
		break;
	}
#endif

	// Free the expression's box and release sub-boxes.
	switch (expr.type) {
	case E_PAIR:
		release_expression(expr.box->car);
		release_expression(expr.box->cdr);
		free(expr.box);
		break;
	case E_MACRO:
	case E_PROCEDURE:
		free(expr.box->params);
		release_expression(expr.box->body);
		release_environment(expr.box->env);
		free(expr.box);
		break;
	default:
		break;
	}
}

struct Expression retain_expression(struct Expression expr) {
	// Increase the reference count of the box.
	switch (expr.type) {
	case E_PAIR:
	case E_MACRO:
	case E_PROCEDURE:
		expr.box->ref_count++;
#if REF_COUNT_LOGGING
		total_ref_count++;
		log_ref_count("retain", expr);
#endif
		break;
	default:
		break;
	}
	return expr;
}

void release_expression(struct Expression expr) {
	// Decrease the reference count of the box, and deallocate if it reaches 0.
	switch (expr.type) {
	case E_PAIR:
	case E_MACRO:
	case E_PROCEDURE:
		assert(expr.box->ref_count > 0);
		expr.box->ref_count--;
#if REF_COUNT_LOGGING
		total_ref_count--;
		log_ref_count("release", expr);
#endif
		if (expr.box->ref_count == 0) {
			dealloc_expression(expr);
		}
		break;
	default:
		break;
	}
}

bool expression_truthy(struct Expression expr) {
	return expr.type != E_BOOLEAN || expr.boolean;
}

bool expression_eq(struct Expression lhs, struct Expression rhs) {
	// Check if they have the same type.
	if (lhs.type != rhs.type) {
		return false;
	}
	// Compare the contents of the expressions.
	switch (lhs.type) {
	case E_NULL:
		return true;
	case E_SYMBOL:
		return lhs.symbol_id == rhs.symbol_id;
	case E_NUMBER:
		return lhs.number == rhs.number;
	case E_BOOLEAN:
		return lhs.boolean == rhs.boolean;
	case E_STDMACRO:
		return lhs.stdmacro == rhs.stdmacro;
	case E_STDPROCMACRO:
	case E_STDPROCEDURE:
		return lhs.stdproc == rhs.stdproc;
	case E_PAIR:
	case E_MACRO:
	case E_PROCEDURE:
		return lhs.box == rhs.box;
	}
}

bool expression_arity(Arity *out, struct Expression expr) {
	switch (expr.type) {
	case E_STDMACRO:
		*out = stdmacro_name_arity[expr.stdmacro].arity;
		return true;
	case E_STDPROCMACRO:
	case E_STDPROCEDURE:
		*out = stdproc_name_arity[expr.stdproc].arity;
		return true;
	case E_MACRO:
	case E_PROCEDURE:
		*out = expr.box->arity;
		return true;
	default:
		return false;
	}
}

bool arity_allows(Arity arity, size_t n_args) {
	if (arity < 0) {
		return n_args >= (size_t)ATLEAST(arity);
	}
	return n_args == (size_t)arity;
}

// Prints a pair to the 'stream', assuming the left parenthesis has already been
// printed. Uses standard Lisp s-expression notation.
static void print_pair(struct Box *box, bool first, FILE *stream) {
	if (!first) {
		putc(' ', stream);
	}
	print_expression(box->car, stream);
	switch (box->cdr.type) {
	case E_NULL:
		putc(')', stream);
		break;
	case E_PAIR:
		print_pair(box->cdr.box, false, stream);
		break;
	default:
		// Print a dot before the last cdr if it is not null.
		fputs(" . ", stream);
		print_expression(box->cdr, stream);
		putc(')', stream);
		break;
	}
}

void print_expression(struct Expression expr, FILE *stream) {
	switch (expr.type) {
	case E_NULL:
		fputs("()", stream);
		break;
	case E_SYMBOL:
		fputs(find_string(expr.symbol_id), stream);
		break;
	case E_NUMBER:
		fprintf(stream, "%ld", expr.number);
		break;
	case E_BOOLEAN:
		fprintf(stream, "#%c", expr.boolean ? 't' : 'f');
		break;
	case E_STDMACRO:
		fprintf(stream, "#<macro %s>",
				stdmacro_name_arity[expr.stdmacro].name);
		break;
	case E_STDPROCMACRO:
		fprintf(stream, "#<macro %s>",
				stdproc_name_arity[expr.stdproc].name);
		break;
	case E_STDPROCEDURE:
		fprintf(stream, "#<procedure %s>",
				stdproc_name_arity[expr.stdproc].name);
		break;
	case E_PAIR:
		putc('(', stream);
		print_pair(expr.box, true, stream);
		break;
	case E_MACRO:
		fprintf(stream, "#<macro %s>", (void *)expr.box);
		break;
	case E_PROCEDURE:
		fprintf(stream, "#<procedure %p>", (void *)expr.box);
		break;
	}
}
