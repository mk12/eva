// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef IMPL_H
#define IMPL_H

#include "expr.h"

// Invokes the implementation for the standard procedure and returns the
// resulting expression. Assumes the application has already been type-checked.
// The standard procedure cannot be S_EVAL, S_APPLY, or S_READ.
struct Expression invoke_implementation(
		enum StandardProcedure stdproc, struct Expression *args, size_t n);

#endif
