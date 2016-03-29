// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef CHECK_H
#define CHECK_H

#include "error.h"
#include "expr.h"

// Returns NULL if the application of 'proc' to 'args' is valid. Allocates and
// returns an evaluation error otherwise, including the case where 'proc' is not
// a procedure.
struct EvalError *check_application(
	struct Expression proc, struct Expression *args, size_t n);

// Returns NULL if 'expr' is a well-formed list. Allocates and returns an
// evaluation error otherwise.
struct EvalError *check_list(struct Expression expr);

#endif
