// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "set.h"

#include "util.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Constants for memory allocation.
#define DEFAULT_CAP 16
#define BITMAP_SIZE 16

// A set consists of a dynamic array of intern identifiers and a fixed-size
// bitmap to make membership checks more efficient.
struct Set {
	size_t cap;
	size_t len;
	InternId *ids;
	uint8_t bitmap[BITMAP_SIZE];
};

struct Set *new_set(void) {
	struct Set *set = xmalloc(sizeof *set);
	set->cap = DEFAULT_CAP;
	set->len = 0;
	set->ids = xmalloc(set->cap * sizeof *set->ids);
	memset(&set->bitmap, 0, BITMAP_SIZE);
	return set;
}

bool add_to_set(struct Set *set, InternId id) {
	size_t h = id % (BITMAP_SIZE * CHAR_BIT);
	size_t index = h / CHAR_BIT;
	size_t mask = 1 << (h - index * CHAR_BIT);
	// Check if the bit is already set.
	if ((set->bitmap[index] & mask) != 0) {
		// It might already be here; check the slow way.
		for (size_t i = 0; i < set->len; i++) {
			if (set->ids[i] == id) {
				return false;
			}
		}
	}
	set->bitmap[index] |= mask;
	// Grow the array if necessary.
	if (set->len >= set->cap) {
		set->cap *= 2;
		set->ids = xrealloc(set->ids, set->cap * sizeof *set->ids);
	}
	// Add the identifier to the array.
	set->ids[set->len++] = id;
	return true;
}

void free_set(struct Set *set) {
	free(set->ids);
	free(set);
}
