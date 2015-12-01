// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "intern.h"

#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE_BITS 10
#define TABLE_SIZE (1 << TABLE_SIZE_BITS)
#define DEFAULT_BUCKET_SIZE 16

struct Bucket {
	int cap;
	int len;
	char **strings;
};

static struct Bucket *table[TABLE_SIZE];

InternID intern_string(const char *str) {
	return intern_string_n(str, strlen(str));
}

InternID intern_string_n(const char *str, int n) {
	InternID h = 5381;
	for (int i = 0; i < n; i++) {
		h = ((h << 5) + h) + str[i];
	}
	h %= TABLE_SIZE;
	struct Bucket *bucket = table + h;

	if (!bucket->strings) {
		bucket->cap = DEFAULT_BUCKET_SIZE;
		bucket->len = 0;
		bucket->strings = malloc(DEFAULT_BUCKET_SIZE * sizeof *bucket->strings);
	}

	for (int i = 0; i < bucket->len; i++) {
		if (strncmp(str, bucket->strings[i], n) == 0) {
			return (i << TABLE_SIZE_BITS) | h;
		}
	}
	
	if (bucket->len >= bucket->cap) {
		bucket->cap *= 2;
		bucket->strings = realloc(
				bucket->strings, bucket->cap * sizeof *bucket->strings);
	}

	char *new_str = malloc(n + 1);
	strncpy(new_str, str, n);
	new_str[n] = '\0';
	int pos = bucket->len;
	bucket->strings[pos] = new_str;
	bucket->len++;
	return (pos << TABLE_SIZE_BITS) | h;
}

const char *find_string(InternID id) {
	int h = id & (TABLE_SIZE - 1);
	int pos = id >> TABLE_SIZE_BITS;
	return intern_table[h].strings[pos];
}
