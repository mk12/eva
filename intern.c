// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "intern.h"

#include <stdlib.h>
#include <string.h>

struct InternList {
	const char *string;
	struct InternList *rest;
};

#define TABLE_SIZE_BITS 10
#define TABLE_SIZE (1 << TABLE_SIZE_BITS)

static struct InternList *intern_table[TABLE_SIZE];

InternID intern_string(const char *str) {
	return intern_string_n(str, strlen(str));
}

InternID intern_string_n(const char *str, int n) {
	InternID h = 5381;
	for (int i = 0; i < n; i++) {
		h = ((h << 5) + h) + str[i];
	}
	h %= TABLE_SIZE;

	struct InternList *new_cell = malloc(sizeof *new_cell);
	new_cell->string = malloc(n + 1);
	strncpy(new_cell->string, str, n);
	new_cell->string[n] = '\0';
	new_cell->rest = NULL;

	int pos = 0;
	struct InternList *cell = intern_table[h];
	if (!cell) {
		intern_table[h] = new_cell;
	} else {
		while (cell->rest) {
			cell = cell->rest;
			pos++;
		}
		cell->rest = new_cell;
	}

	return (pos << TABLE_SIZE_BITS) | h;
}

const char *find_string(InternID id) {
	int h = id & (TABLE_SIZE - 1);
	int pos = id >> TABLE_SIZE_BITS;
	struct InternList *cell = intern_table[h];
	for (int i = 0; i < pos; i++) {
		cell = cell->rest;
	}
	return cell->string;
}
