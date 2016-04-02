// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "list.h"

bool check_list(struct Expression expr) {
	while (expr.type != E_NULL) {
		if (expr.type != E_PAIR) {
			return false;
		}
		expr = expr.box->pair.cdr;
	}
	return true;
}

static struct ArrayResult sexpr_array(struct Expression list, bool dot) {
	struct ArrayResult result;
	result.size = 0;
	result.dot = false;
	result.exprs = NULL;

	// Count the number of elements in the list.
	struct Expression expr = list;
	while (expr.type != E_NULL) {
		if (expr.type != E_PAIR) {
			if (dot) {
				result.dot = true;
				result.size++;
				break;
			} else {
				// Malformed list. Return NULL 'exprs'.
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
