// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef UTIL_H
#define UTIL_H

// Safe versions of standard memory allocation functions. If the allocation
// fails, they print an error message and abort.
void *safe_malloc(size_t size);
void *safe_calloc(size_t size);
void *safe_realloc(void *ptr, size_t size);

// Reads the contents of a file into a null-terminated string. On success,
// returns the string. Otherwise, returns NULL and sets the global 'errno'.
char *read_file(const char *filename);

#endif
