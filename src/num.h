// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef NUM_H
#define NUM_H

#include "expr.h"

// A NumFunction takes an array of 'n' expressions (all assumed to be E_NUMBER),
// computes some operation on them, and returns the result as an expression.
typedef struct Expression (*NumFunction)(struct Expression *args, size_t n);

// Numeric predicates that take a variable number of arguments.
struct Expression num_eq(struct Expression *args, size_t n);
struct Expression num_lt(struct Expression *args, size_t n);
struct Expression num_gt(struct Expression *args, size_t n);
struct Expression num_le(struct Expression *args, size_t n);
struct Expression num_ge(struct Expression *args, size_t n);

// Numeric operations that take a variable number of arguments.
struct Expression num_add(struct Expression *args, size_t n);
struct Expression num_sub(struct Expression *args, size_t n);
struct Expression num_mul(struct Expression *args, size_t n);
struct Expression num_div(struct Expression *args, size_t n);

// Numeric operations that take exactly two arguments (assumes 'n' is 2).
struct Expression num_rem(struct Expression *args, size_t n);
struct Expression num_mod(struct Expression *args, size_t n);
struct Expression num_expt(struct Expression *args, size_t n);

#endif
