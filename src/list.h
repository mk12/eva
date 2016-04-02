// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef LIST_H
#define LIST_H

#include "expr.h"

// ArrayResult contains the result of converting an s-expression list to an
// array of expressions. The 'exprs' field is NULL if the list was malformed.
struct ArrayResult {
	bool dot;                 // whether the list was dotted
	size_t size;              // number of expressions in the array
	struct Expression *exprs; // expression array or NULL
};

// Returns true if the expression is a well-formed list.
bool well_formed_list(struct Expression expr);

// Converts an s-expression list to a flat array of expressions. Copies elements
// of the list directly to the new array, but does not alter reference counts.
// Stores NULL in the 'exprs' field of the result if the list is malformed.
//
// If 'dot' is true, then "dotted" lists such as (1 2 . 3) are also allowed,
// where the final cdr is considered the final element of the array. This also
// means that a single non-list value will be returned as a singleton array,
// instead of causing an error, which it will when 'dot' is false.
static struct ArrayResult sexpr_array(struct Expression list, bool dot) {

#endif
