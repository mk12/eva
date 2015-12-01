// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "env.h"

#include "expr.h"
#include "intern.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_TABLE_SIZE 1024
#define DEFAULT_BUCKET_SIZE 16

struct Entry {
	InternID key;
	struct Expression expr;
};

struct Bucket {
	int cap;
	int len;
	struct Entry *entries;
};

struct Environment {
	int size;
	int total_entries;
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
	for (int i = 0; i < N_SPECIAL_PROCS; i++) {
		bind(env, intern_string(special_name(i)), new_special(i));
	}
	return env;
}

struct LookupResult lookup(struct Environment *env, InternID key) {
	int index = key % env->size;
	int len = env->table[index].len;
	struct Entry *ents = env->table[index].entries;
	for (int i = len - 1; i >= 0; i--) {
		if (ents[i].key == key) {
			return (struct LookupResult){ .found = true, .expr = ents[i].expr };
		}
	}
	return (struct LookupResult){ .found = false };
}

static void bind_unchecked(
		struct Environment *env, InternID key, struct Expression expr) {
	struct Bucket *bucket = env->table + (key % env->size);
	if (!bucket->entries) {
		bucket->cap = DEFAULT_BUCKET_SIZE;
		bucket->entries = malloc(bucket->cap * sizeof *bucket->entries);
	} else if (bucket->len >= bucket->cap) {
		bucket->cap *= 2;
		bucket->entries = realloc(
				bucket->entries, bucket->cap * sizeof *bucket->entries);
	}
	bucket->entries[bucket->len].key = key;
	bucket->entries[bucket->len].expr = retain_expression(expr);
	bucket->len++;
	env->total_entries++;
}

void bind(struct Environment *env, InternID key, struct Expression expr) {
	if (3 * env->size >= 4 * env->total_entries) {
		int old_size = env->size;
		struct Bucket *old_table = env->table;
		env->size *= 2;
		env->table = calloc(env->size, sizeof *env->table);
		for (int i = 0; i < old_size; i++) {
			for (int j = 0; j < old_table[i].len; j++) {
				struct Entry ent = old_table[i].entries[j];
				bind_unchecked(env, ent.key, ent.expr);
			}
			if (old_table[i].entries) {
				free(old_table[i].entries);
			}
		}
		free(old_table);

	}
	bind_unchecked(env, key, expr);
}

void unbind(struct Environment *env, InternID key) {
	int index = key % env->size;
	int len = env->table[index].len;
	struct Entry *ents = env->table[index].entries;
	bool shift = false;
	for (int i = 0; i < len; i++) {
		if (shift) {
			ents[i-1].key = ents[i].key;
			ents[i-1].expr = ents[i].expr;
		} else {
			if (ents[i].key == key) {
				shift = true;
			}
		}
	}
	release_expression(env->table[index].expr);
	env->table[index].len--;
	env->total_entries--;
}

void unbind_last(struct Environment *env, InternID key) {
	int index = key % env->size;
	int len = env->table[index].len;
	struct Entry *ents = env->table[index];
	release_expression(ents[len-1].expr);
	env->table[index].len--;
	env->total_entries--;
}
