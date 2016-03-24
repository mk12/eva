// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "intern.h"

#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE_BITS 10
#define TABLE_SIZE (1 << TABLE_SIZE_BITS)
#define DEFAULT_BUCKET_SIZE 16

struct Bucket {
	size_t cap;
	size_t len;
	char **strings;
};

static struct Bucket table[TABLE_SIZE];

InternID intern_string(const char *str) {
	return intern_string_n(str, strlen(str));
}

InternID intern_string_n(const char *str, size_t n) {
	// Calculate the hash value of the string.
	InternID h = 5381;
	for (size_t i = 0; i < n; i++) {
		h = ((h << 5) + h) + (InternID)str[i];
	}
	h %= TABLE_SIZE;

	// Look up the bucket. Initialize it if it is empty.
	struct Bucket *bucket = table + h;
	if (!bucket->strings) {
		bucket->cap = DEFAULT_BUCKET_SIZE;
		bucket->len = 0;
		bucket->strings = malloc(DEFAULT_BUCKET_SIZE * sizeof *bucket->strings);
	}

	// Check if the same string has already been interned.
	for (size_t i = 0; i < bucket->len; i++) {
		if (strncmp(str, bucket->strings[i], n) == 0) {
			// If so, return its intern ID.
			return (InternID)(i << TABLE_SIZE_BITS) | h;
		}
	}
	
	// Double the array's capacity if necessary.
	if (bucket->len >= bucket->cap) {
		bucket->cap *= 2;
		bucket->strings = realloc(
				bucket->strings, bucket->cap * sizeof *bucket->strings);
	}

	// Copy the string and add a null terminator.
	char *new_str = malloc(n + 1);
	strncpy(new_str, str, n);
	new_str[n] = '\0';
	// Add the string to the bucket.
	size_t pos = bucket->len;
	bucket->strings[pos] = new_str;
	// Update the count.
	bucket->len++;

	// Create an intern ID by combining the position bits and hash bits.
	return (InternID)(pos << TABLE_SIZE_BITS) | h;
}

const char *find_string(InternID id) {
	// Decode the hash value and the position.
	int h = id & (TABLE_SIZE - 1);
	int pos = id >> TABLE_SIZE_BITS;
	// Look up the interned string.
	return table[h].strings[pos];
}
