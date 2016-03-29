// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "env.h"

#include "expr.h"
#include "intern.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_TABLE_SIZE 1024
#define DEFAULT_BUCKET_SIZE 16

struct Entry {
	InternId key;
	struct Expression expr;
};

struct Bucket {
	size_t cap;
	size_t len;
	struct Entry *entries;
};

struct Environment {
	size_t size;
	size_t total_entries;
	struct Bucket *table;
};

struct Environment *empty_environment(void) {
	struct Environment *env = malloc(sizeof *env);
	env->size = DEFAULT_TABLE_SIZE;
	env->total_entries = 0;
	env->table = calloc(env->size, sizeof *env->table);
	return env;
}

struct Environment *default_environment(void) {
	struct Environment *env = empty_environment();
	// Bind all special procedure names to their expressions.
	for (int i = 0; i < N_SPECIAL_PROCS; i++) {
		enum SpecialType type = (enum SpecialType)i;
		bind(env, intern_string(special_name(type)), new_special(type));
	}
	// Bind the symbol "else" to true (used in the cond special form).
	bind(env, intern_string("else"), new_boolean(true));
	return env;
}

struct LookupResult lookup(struct Environment *env, InternId key) {
	// Look up the bucket corresponding to the key.
	size_t index = key % env->size;
	size_t len = env->table[index].len;
	// Check each entry in the bucket.
	struct Entry *ents = env->table[index].entries;
	for (size_t i = 1; i <= len; i++) {
		if (ents[len-i].key == key) {
			return (struct LookupResult){
				.found = true,
				.expr = ents[len-i].expr
			};
		}
	}
	return (struct LookupResult){ .found = false };
}

static void bind_unchecked(
		struct Environment *env, InternId key, struct Expression expr) {
	// Look up the bucket corresponding to the key.
	struct Bucket *bucket = env->table + (key % env->size);
	if (!bucket->entries) {
		// Initialize the bucket if it is empty.
		bucket->cap = DEFAULT_BUCKET_SIZE;
		bucket->entries = malloc(bucket->cap * sizeof *bucket->entries);
	} else if (bucket->len >= bucket->cap) {
		// Grow the array if necessary.
		bucket->cap *= 2;
		bucket->entries = realloc(
				bucket->entries, bucket->cap * sizeof *bucket->entries);
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
		env->table = calloc(env->size, sizeof *env->table);
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

void unbind(struct Environment *env, InternId key) {
	// Look up the bucket corresponding to the key.
	size_t index = key % env->size;
	size_t len = env->table[index].len;
	// Check each entry in the bucket.
	struct Entry *ents = env->table[index].entries;
	bool shift = false;
	for (size_t i = 0; i < len; i++) {
		if (shift) {
			ents[i-1].key = ents[i].key;
			ents[i-1].expr = ents[i].expr;
		} else {
			if (ents[i].key == key) {
				release_expression(ents[i].expr);
				shift = true;
			}
		}
	}
	// Update the counts if an expression was unbound.
	if (shift) {
		env->table[index].len--;
		env->total_entries--;
	}
}

void unbind_last(struct Environment *env, InternId key) {
	// Look up the bucket corresponding to the key.
	size_t index = key % env->size;
	size_t len = env->table[index].len;
	// Unbind the most recently bound expression.
	release_expression(env->table[index].entries[len-1].expr);
	// Update the counts.
	env->table[index].len--;
	env->total_entries--;
}

void free_environment(struct Environment *env) {
	for (size_t i = 0; i < env->size; i++) {
		size_t len = env->table[i].len;
		struct Entry *ents = env->table[i].entries;
		for (size_t j = 0; j < len; j++) {
			release_expression(ents[j].expr);
		}
		free(ents);
	}
	free(env);
}
