// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "intern.h"

#include <stdlib.h>
#include <string.h>

// Constants for the intern table.
#define TABLE_SIZE_BITS 10
#define TABLE_SIZE (1 << TABLE_SIZE_BITS)
#define DEFAULT_BUCKET_CAP 16

// A bucket contains two dynamic arrays that grow together: one for strings, and
// one for metadata bytes. It doubles the capacity of each array whenever the
// length is about to exceed the capacity.
struct Bucket {
	size_t cap;
	size_t len;
	const char **strings;
	uint8_t *meta;
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
		bucket->meta = malloc(DEFAULT_BUCKET_CAP * sizeof *bucket->meta);
	}

	// Check if the same string has already been interned.
	for (size_t i = 0; i < bucket->len; i++) {
		if (strncmp(str, bucket->strings[i], n) == 0) {
			return (InternId)(i << TABLE_SIZE_BITS) | h;
		}
	}
	
	// Double the capacity of each array if necessary.
	if (bucket->len >= bucket->cap) {
		bucket->cap *= 2;
		bucket->strings = realloc(bucket->strings,
				bucket->cap * sizeof *bucket->strings);
		bucket->meta = realloc(bucket->meta,
				bucket->cap * sizeof *bucket->meta);
	}

	// Copy the string and add a null terminator.
	char *new_str = malloc(n + 1);
	strncpy(new_str, str, n);
	new_str[n] = '\0';
	// Add the string and default metadata to the bucket.
	size_t pos = bucket->len;
	bucket->strings[pos] = new_str;
	bucket->meta[pos] = 0;
	bucket->len++;

	// Create an intern ID by combining the position bits and hash bits.
	return (InternId)(pos << TABLE_SIZE_BITS) | h;
}

const char *find_string(InternId id) {
	size_t h = id & (TABLE_SIZE - 1);
	size_t pos = id >> TABLE_SIZE_BITS;
	return table[h].strings[pos];
}

void set_metadata(InternId id, uint8_t metadata) {
	size_t h = id & (TABLE_SIZE - 1);
	size_t pos = id >> TABLE_SIZE_BITS;
	table[h].meta[pos] = metadata;
}

uint8_t find_metadata(InternId id) {
	size_t h = id & (TABLE_SIZE - 1);
	size_t pos = id >> TABLE_SIZE_BITS;
	return table[h].meta[pos];
}
