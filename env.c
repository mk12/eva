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
	Expression expr;
};

struct Bucket {
	int size;
	int len;
	struct Entry *entries;
};

struct Environment {
	int size;
	int count;
	struct Bucket *table;
};

struct Environment *empty_environment(void) {
	struct Environment *env = malloc(sizeof *env);
	env->size = DEFAULT_TABLE_SIZE;
	env->count = 0;
	env->table = calloc(env->size, sizeof *env->table);
	return env;
}

struct Environment *default_environment(void) {
	struct Environment *env = empty_environment();
	for (int i = 0; i < N_SPECIAL_PROCS; i++) {
		env = bind(env, intern_string(special_name(i)), new_special(i));
	}
	return env;
}

struct LookupResult lookup(struct Environment *env, InternID key) {
	struct Bucket bucket = env->table[key % env->size];
	for (int i = 0; i < bucket.len; i++) {
		if (bucket[i].key == key) {
			return { .found = true, .expr = bucket[i].expr };
		}
	}
	return { .found = false };
}

static void bind_unchecked(
		struct Environment *env, InternID key, struct Expression expr) {
	struct Bucket *bucket = env->table + (key % env->size);
	if (!bucket->entries) {
		bucket->size = DEFAULT_BUCKET_SIZE;
		bucket->entries = malloc(bucket->size * sizeof *bucket->entries);
	} else if (bucket->len >= bucket->size) {
		bucket->size *= 2;
		bucket->entries = realloc(
				bucket->entries, bucket->size * sizeof *bucket->entries);
	}
	bucket->entries[bucket->len].key = key;
	bucket->entries[bucket->len].expr = expr;
	bucket->len++;
}

void bind(struct Environment *env, InternID key, struct Expression expr) {
	if (3 * env->size >= 4 * env->count) {
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
	struct Bucket bucket = env->table[key % env->size];
	bool shift = false;
	for (int i = 0; i < bucket.len; i++) {
		if (shift) {
			bucket[i-1].key = bucket[i].key;
			bucket[i-1].expr = bucket[i].expr;
		} else {
			if (bucket[i].key == key) {
				shift = true;
			}
		}
	}
	bucket.len--;
	env->count--;
}

void unbind_last(struct Environment *env, InternID key) {
	struct Bucket bucket = env->table[key % env->size];
	bucket.len--;
	env->count--;
}
