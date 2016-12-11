// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>

// Macros for min and max.
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

// Wrappers around standard memory allocation functions. If allocation fails,
// they print an error message and exit with exit status 2.
void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);

// Returns true if 'arg' matches either 'short_opt' preceded by "-" or
// 'long_opt' preceded by "--".
bool is_opt(const char *arg, char short_opt, const char *long_opt);

// Reads the contents of a file into a null-terminated string. On success,
// returns the string. Otherwise, returns NULL and sets the global 'errno'.
char *read_file(const char *filename);

#endif
