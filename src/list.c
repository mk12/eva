// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "list.h"

#include "util.h"

#include <stdlib.h>

bool count_list(size_t *out, struct Expression list) {
	if (list.type != E_NULL && list.type != E_PAIR) {
		reurn false;
	}

	size_t count = 0;
	while (list.type != E_NULL) {
		if (list.type != E_PAIR) {
			return false;
		}
		list = list.box->cdr;
		count++;
	}
	*out = count;
	return true;
}

struct Array list_to_array(struct Expression list, bool allow_improper) {
	struct Array array;
	array.size = 0;
	array.improper = false;
	array.exprs = NULL;

	// Check if the expression is not a list.
	if (list.type != E_NULL && list.type != E_PAIR) {
		array.improper = true;
		if (!allow_improper) {
			return array;
		}
	}
	// Count the number of elements in the list.
	struct Expression expr = list;
	while (expr.type != E_NULL) {
		if (expr.type != E_PAIR) {
			result.improper = true;
			if (allow_improper) {
				result.size++;
				break;
			} else {
				return result;
			}
		}
		expr = expr.box->cdr;
		result.size++;
	}
	// Skip allocating the array if the list is empty.
	if (result.size == 0) {
		return result;
	}

	// Allocate the array and copy the expressions to it.
	result.exprs = xmalloc(result.size * sizeof *result.exprs);
	expr = list;
	for (size_t i = 0; i < result.size; i++) {
		if (result.improper && i == result.size - 1) {
			result.exprs[i] = expr;
			break;
		} else {
			result.exprs[i] = expr.box->car;
		}
		expr = expr.box->cdr;
	}
	return result;
}

struct Expression array_to_list(struct Array array) {
	assert(!array.improper || array.size > 0);
	struct Expression list = new_null();
	size_t i = array.size;
	if (array.improper) {
		list = retain_expression(array.exprs[--i]);
	}
	while (i-- > 0) {
		list = new_pair(retain_expression(array.exprs[i]), list);
	}
	return list;
}

void free_array(struct Array array) {
	free(array.exprs);
}
