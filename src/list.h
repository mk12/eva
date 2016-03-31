// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef LIST_H
#define LIST_H

#include "expr.h"

// ...
struct ArrayResult {
	size_t size;
	bool dot;
	struct Expression *exprs;
	const char *err_msg;
};

// Returns NULL if the expression is a well-formed list. Otherwise, allocates
// and returns an evaluation error of type ERR_SYNTAX.
bool check_list(struct Expression expr);

// Converts a list to a flat array of expressions. Returns an error messge if
// the expression is not a well-formed list. Copies elements of the list
// directly to the new array, but does not alter reference counts.
//
// If dot is true, then lists such as (1 2 . 3) are also allowed, where the
// final cdr is the final element of the array, not the null value. This also
// means that a single non-list value will be returned as a singleton array
// (instead of causing an error, which it will when dot is false).
static struct ArrayResult sexpr_array(struct Expression list, bool dot) {

#endif
