// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef MACRO_H
#define MACRO_H

#include "eval.h"
#include "expr.h"

#include <stddef.h>

struct Environment;

// Invokes the implementation for the standard macro applied to 'args' (an array
// of 'n' arguments). Assumes the application has already been type-checked. On
// success, returns the resulting expression. Otherwise, allocates and returns
// an evaluation error. The standard macro cannot be F_QUASIQUOTE, F_UNQUOTE, or
// F_UNQUOTE_SPLICING.
struct EvalResult invoke_stdmacro(
		enum StandardMacro stdmacro,
		struct Expression *args,
		size_t n,
		struct Environment *env);

#endif
