// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef TYPE_H
#define TYPE_H

#include "expr.h"

#include <stddef.h>

struct EvalError;

// Type-checks the application of 'expr' to 'args' (an array of 'n' arguments).
// Assumes 'expr' is callable and accepts 'n' arguments. Returns NULL if all
// arguments have the correct types. Allocates and returns a type error
// otherwise. Note that the expressions in 'args' should be evaluated if 'expr'
// is a procedure, but unevaluated if 'expr' is a macro.
struct EvalError *type_check(
		struct Expression expr, struct Expression *args, size_t n);

#endif
