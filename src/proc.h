// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef PROC_H
#define PROC_H

#include "expr.h"

#include <stddef.h>

// Invokes the implementation for the standard procedure applied to 'args' (an
// array of 'n' arguments), and returns the resulting expression. Assumes the
// application has already been type-checked. The standard procedure cannot be
// S_EVAL, S_APPLY, S_READ, S_ERROR, or S_LOAD.
struct Expression invoke_stdprocedure(
		enum StandardProcedure stdproc, struct Expression *args, size_t n);

#endif
