// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef LIST_H
#define LIST_H

#include "expr.h"

#include <stdbool.h>

// ArrayResult contains the result of converting an s-expression list to an
// array of expressions. For zero-length lists, 'size' is 0 and 'exprs' is NULL.
// The 'exprs' field does not have a meaninful value if 'improper' is true but
// the second argument in 'list_to_array' was false.
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
// are accepted in addition to proper lists.
struct ArrayResult list_to_array(struct Expression list, bool improper);

#endif
