// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef LIST_H
#define LIST_H

#include "expr.h"

#include <stdbool.h>

// A flat array of expressions from a list. For empty lists, 'size' is 0 and
// 'exprs' is NULL. For improper lists (including single non-list values),
// 'improper' is true and the last element of 'exprs' is the final cdr.
struct Array {
	bool improper;
	size_t size;
	struct Expression *exprs;
};

// Counts the elements of a list. If it is a well-formed list (not improper),
// returns true and stores its length in 'out'. Otherwise, returns false.
bool count_list(size_t *out, struct Expression list);

// If 'lhs' is a well-formed nonempty list, creates a copy of it with 'rhs'
// concatenated on the end ('rhs' is reused by retaining), stores it in 'out',
// and returns true. Otherwise, returns false.
bool concat_list(
		struct Expression *out, struct Expression lhs, struct Expression rhs);

// Converts a list to an array. Copies elements of the list directly into the
// new array without altering reference counts. If 'allow_improper' is false,
// skips allocation and stores NULL in the 'exprs' field for improper lists.
// Note that empty lists also result in NULL, but they set 'improper' to false.
struct Array list_to_array(struct Expression list, bool allow_improper);

// Converts an array back to a list. Sets the reference count to 1. Retains all
// expressions in the array. For any expression 'expr', the result of
// 'array_to_list(list_to_array(expr, true))' is recursively equal to 'expr' in
// the sense of the Scheme predicate "equal?".
struct Expression array_to_list(struct Array array);

// Checks for duplicates in 'array'. If it finds a duplicate, returns true and
// stores it in 'out'. Otherwise, returns false. Assumes all the expressions in
// 'array.exprs' are of type E_SYMBOL.
bool find_duplicate_symbol(InternId *out, struct Array array);

// Frees the memory allocated by 'list_to_array'.
void free_array(struct Array array);

#endif
