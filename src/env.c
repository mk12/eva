// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "env.h"

#include "intern.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Constants for memory allocation.
#define BASE_TABLE_SIZE 1024
#define BASE_BUCKET_SIZE 8
#define CHILD_BUCKET_SIZE 2

// An entry maps an intern identifier to an expression.
struct Entry {
	InternId key;
	struct Expression expr;
};

// A bucket is a dynamic array of entries.
struct Bucket {
	size_t cap;
	size_t len;
	struct Entry *entries;
};

// An environment is a collection of variable bindings. It is implemented as a
// dynamic hash table that maps keys (interned strings) to expressions.
struct Environment {
	int ref_count;
	struct Environment *parent;
	size_t size;
	size_t total_entries;
	struct Bucket *table;
};

struct Environment *new_base_environment(void) {
	struct Environment *env = xmalloc(sizeof *env);
	env->ref_count = 1;
	env->parent = NULL;
	env->size = BASE_TABLE_SIZE;
	env->total_entries = 0;
	env->table = xcalloc(env->size, sizeof *env->table);
	return env;
}

struct Environment *new_environment(
		struct Environment *parent, size_t size_estimate) {
	assert(size_estimate > 0);
	struct Environment *env = xmalloc(sizeof *env);
	env->ref_count = 1;
	env->parent = retain_environment(parent);
	env->size = size_estimate * 2;
	env->total_entries = 0;
	env->table = xcalloc(env->size, sizeof *env->table);
	return env;
}

static void dealloc_environment(struct Environment *env) {
	for (size_t i = 0; i < env->size; i++) {
		size_t len = env->table[i].len;
		struct Entry *ents = env->table[i].entries;
		for (size_t j = 0; j < len; j++) {
			release_expression(ents[j].expr);
		}
		free(ents);
	}
	release_environment(env->parent);
	free(env->table);
	free(env);
}

struct Environment *retain_environment(struct Environment *env) {
	if (env) {
		env->ref_count++;
	}
	return env;
}

void release_environment(struct Environment *env) {
	if (env) {
		assert(env->ref_count > 0);
		env->ref_count--;
		if (env->ref_count == 0) {
			dealloc_environment(env);
		}
	}
}

struct Expression *lookup(const struct Environment *env, InternId key) {
	while (env) {
		// Look up the bucket corresponding to the key.
		size_t index = key % env->size;
		size_t len = env->table[index].len;
		// Check each entry in the bucket.
		struct Entry *ents = env->table[index].entries;
		for (size_t i = 0; i < len; i++) {
			if (ents[i].key == key) {
				return &ents[i].expr;
			}
		}
		// Check the parent environment next.
		env = env->parent;
	}
	return NULL;
}

static void bind_unchecked(
		struct Environment *env, InternId key, struct Expression expr) {
	env->total_entries++;
	struct Bucket *bucket = env->table + (key % env->size);
	if (!bucket->entries) {
		// Initialize the bucket if it is empty.
		bucket->cap = env->parent ? CHILD_BUCKET_SIZE : BASE_BUCKET_SIZE;
		bucket->entries = xmalloc(bucket->cap * sizeof *bucket->entries);
	} else {
		// Check if the variable is already bound.
		for (size_t i = 0; i < bucket->len; i++) {
			if (bucket->entries[i].key == key) {
				release_expression(bucket->entries[i].expr);
				bucket->entries[i].expr = retain_expression(expr);
				return;
			}
		}
		// Grow the array if necessary.
		if (bucket->len >= bucket->cap) {
			bucket->cap *= 2;
			bucket->entries = xrealloc(bucket->entries,
					bucket->cap * sizeof *bucket->entries);
		}
	}
	// Add an entry to the end to bind the expression.
	bucket->entries[bucket->len].key = key;
	bucket->entries[bucket->len].expr = retain_expression(expr);
	// Update the counts.
	bucket->len++;
	env->total_entries++;
}

void bind(struct Environment *env, InternId key, struct Expression expr) {
	// Check if the load factor is greater than 0.75.
	if (4 * env->total_entries >= 3 * env->size) {
		size_t old_size = env->size;
		struct Bucket *old_table = env->table;
		// Create a table with double the number of buckets.
		env->size *= 2;
		env->table = xcalloc(env->size, sizeof *env->table);
		// Bind all the expressions into the new table.
		for (size_t i = 0; i < old_size; i++) {
			for (size_t j = 0; j < old_table[i].len; j++) {
				struct Entry ent = old_table[i].entries[j];
				bind_unchecked(env, ent.key, ent.expr);
			}
			if (old_table[i].entries) {
				free(old_table[i].entries);
			}
		}
		free(old_table);
	}
	// Bind the new expression.
	bind_unchecked(env, key, expr);
}
