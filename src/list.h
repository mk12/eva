// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef LIST_H
#define LIST_H

#include "expr.h"

// ArrayResult contains the result of converting an s-expression list to an
// array of expressions. The 'exprs' field is NULL if the list was improper and
// improper lists were not desired.
struct ArrayResult {
	bool improper;            // whether the list was improper
	size_t size;              // number of expressions in the array
	struct Expression *exprs; // expression array or NULL
};

// Returns true if 'expr' is a well-formed list.
bool well_formed_list(struct Expression expr);

// Counts the elements of the list 'expr'. If it is a well-formed list, returns
// true and stores its length in 'out'. Otherwise, returns false.
bool count_list(size_t *out, struct Expression expr);

// Converts an s-expression list to a flat array of expressions. Copies elements
// of the list directly to the new array, but does not alter reference counts.
// If 'improper' is true, then improper lists (including single non-pair values)
// are accepted. Otherwise, stores NULL in the 'exprs' field for improper lists.
struct ArrayResult list_to_array(struct Expression list, bool improper);

#endif
