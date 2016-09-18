// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef SET_H
#define SET_H

#include "intern.h"

#include <stdbool.h>

struct Set;

// Creates a new empty set.
struct Set *new_set(void);

// Attempts to add 'id' to the set. If 'id' is not already in the set, adds it
// and returns true. Otherwise, returns false.
bool add_to_set(struct Set *set, InternId id);

// Frees the memory associated with a set.
void free_set(struct Set *set);

#endif
