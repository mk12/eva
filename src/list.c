// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "list.h"

// 'ill-formed-list' error in 3 places:
//   1. (f a b . c) invalid
//   2. (apply f '(a b . c)) invalid (if not handled, would reduce to #1 anyway)
//   3. check_list used for the rest of a cond
// perhaps check_list should return boolean?
// we need separate error for invalid method invocation, then the cond one could
// just be a syntax error (ill formed special form)
// but it would be nice for the invalid list to also be a "syntax" erro since it
// is syntax........
const char *check_list(struct Expression expr) {
	while (expr.type != E_NULL) {
		if (expr.type != E_PAIR) {
			return err_ill_list;
		}
		expr = expr.box->pair.cdr;
	}
	return NULL;
}

static struct ArrayResult sexpr_array(struct Expression list, bool dot) {
	struct ArrayResult result;
	result.size = 0;
	result.dot = false;
	result.exprs = NULL;
	result.err_msg = NULL;

	// Count the number of elements in the list.
	struct Expression expr = list;
	while (expr.type != E_NULL) {
		if (expr.type != E_PAIR) {
			if (dot) {
				result.dot = true;
				result.size++;
				break;
			} else {
				result.err_msg = err_ill_list;
				return result;
			}
		}
		expr = expr.box->pair.cdr;
		result.size++;
	}
	// Skip allocating the array if the list is empty.
	if (result.size == 0) {
		return result;
	}

	// Allocate the array.
	result.exprs = malloc(result.size * sizeof *result.exprs);
	// Return to the beginning and copy the expressions to the array.
	expr = list;
	for (size_t i = 0; i < result.size; i++) {
		if (result.dot && i == result.size - 1) {
			// Special case: in a list such as (1 2 . 3), the final element 3 is
			// not a car but rather the final cdr itself.
			result.exprs[i] = expr;
			break;
		} else {
			result.exprs[i] = expr.box->pair.car;
		}
		expr = expr.box->pair.cdr;
	}
	return result;
}

