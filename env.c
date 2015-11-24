// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "env.h"

#include "expr.h"
#include "intern.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_SIZE_BITS 10
#define DEFAULT_SIZE (1 << DEFAULT_SIZE_BITS)

struct Markers;
struct VarChain;
struct ShadowChain;

struct Environment {
	int size;
	struct Markers *table;
};

struct Markers {
	struct VarChain *start;
	struct VarChain *end;
};

struct VarChain {
	InternID key;
	struct Expression expr;
	struct ShadowChain *shadow_start;
	struct ShadowChain *shadow_end;
	struct VarChain *rest;
};

struct ShadowChain {
	struct Expression expr;
	struct ShadowChain *rest;
};

struct Environment *empty_environment(void) {
	struct Environment *env = malloc(sizeof *env);
	env->size = DEFAULT_SIZE;
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
	int i = key % env->size;
	struct VarChain *var = env->table[i].start;
	while (var && var->key != key) {
		var = var->rest;
	}
	if (var) {
		return (struct LookupResult){ .found = true, .expr = var->expr };
	}
	return (struct LookupResult){ .found = false };
}

void bind(struct Environment *env, InternID key, struct Expression expr) {
	int i = key % env->size;

	if (!env->table[i].start) {
		struct VarChain *var = malloc(sizeof *var);
		var->key = key;
		var->expr = retain_expression(expr);
		var->shadow_start = NULL;
		var->shadow_end = NULL;
		var->rest = NULL;
		env->table[i].start = var;
		env->table[i].end = var;
		return;
	}

	struct VarChain *var = env->table[i].start;
	while (var && var->key != key) {
		var = var->rest;
	}
	if (var) {
		struct ShadowChain *shadow = malloc(sizeof *shadow);
		shadow->expr = var->expr;
		shadow->rest = var->shadow_start;
		var->shadow_start = shadow;
		var->expr = retain_expression(expr);
	} else {
		struct VarChain *var = malloc(sizeof *var);
		var->key = key;
		var->expr = retain_expression(expr);
		var->shadow_start = NULL;
		var->shadow_end = NULL;
		var->rest = NULL;
		env->table[i].end->rest = var;
		env->table[i].end = var;
	}
}

void unbind(struct Environment *env, InternID key) {
	int i = key % env->size;
	struct VarChain *prev = NULL;
	struct VarChain *var = env->table[i].start;
	while (var && var->key != key) {
		prev = var;
		var = var->rest;
	}
	assert(var);
	if (var->shadow_start) {
		struct ShadowChain *temp = var->shadow_start;
		release_expression(var->expr);
		var->expr = temp->expr;
		var->shadow_start = temp->rest;
		free(temp);
	} else {
		release_expression(var->expr);
		free(var);
		if (prev) {
			prev->rest = NULL;
			env->table[i].end = prev;
		} else {
			env->table[i].start = NULL;
			env->table[i].end = NULL;
		}
	}
}
