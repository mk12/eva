// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "intern.h"

#include <stdlib.h>
#include <string.h>

// Constants for the intern table.
#define TABLE_SIZE_BITS 10
#define TABLE_SIZE (1 << TABLE_SIZE_BITS)
#define DEFAULT_BUCKET_CAP 16

// A bucket is a dynamic array of strings. It begins with a default capacity,
// and it doubles its capacity every time the length would exceed it.
struct Bucket {
	size_t cap;
	size_t len;
	const char **strings;
};

// The intern table for the program is a static array of buckets. The number of
// buckets is fixed so that the index into the bucket can be encoded into the
// intern identifier. This means looking up a string is a single memory access.
// The downside to this is that interning new strings slows down over time,
// because we have to compare with more strings when checking if the string has
// already been interned.
static struct Bucket table[TABLE_SIZE];

InternId intern_string(const char *str) {
	return intern_string_n(str, strlen(str));
}

InternId intern_string_n(const char *str, size_t n) {
	// Calculate the hash value of the string.
	InternId h = 5381;
	for (size_t i = 0; i < n; i++) {
		h = ((h << 5) + h) + (InternId)str[i];
	}
	h %= TABLE_SIZE;

	// Look up the bucket. Initialize it if it is empty.
	struct Bucket *bucket = table + h;
	if (!bucket->strings) {
		bucket->cap = DEFAULT_BUCKET_CAP;
		bucket->len = 0;
		bucket->strings = malloc(DEFAULT_BUCKET_CAP * sizeof *bucket->strings);
	}

	// Check if the same string has already been interned.
	for (size_t i = 0; i < bucket->len; i++) {
		if (strncmp(str, bucket->strings[i], n) == 0) {
			return (InternId)(i << TABLE_SIZE_BITS) | h;
		}
	}
	
	// Double the array's capacity if necessary.
	if (bucket->len >= bucket->cap) {
		bucket->cap *= 2;
		bucket->strings = realloc(bucket->strings,
				bucket->cap * sizeof *bucket->strings);
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
	return (InternId)(pos << TABLE_SIZE_BITS) | h;
}

const char *find_string(InternId id) {
	// Decode the hash value and the position.
	size_t h = id & (TABLE_SIZE - 1);
	size_t pos = id >> TABLE_SIZE_BITS;
	// Look up the interned string.
	return table[h].strings[pos];
}
