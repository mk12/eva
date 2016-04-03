// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef TYPE_H
#define TYPE_H

#include "expr.h"

struct EvalError;

// Type-checks the application of 'proc' to 'args' (an array of 'n' arguments).
// Returns NULL if it is valid. Allocates and returns a type error if 'proc' is
// not a procedure, if 'n' does not match the arity of 'proc', or if the
// arguments are not all of the correct types.
struct EvalError *type_check(
	struct Expression proc, struct Expression *args, size_t n);

#endif
