// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef UTIL_H
#define UTIL_H

// Wrappers around standard memory allocation functions. If allocation fails,
// they print an error message and exit with exit status 2.
void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);

// Reads the contents of a file into a null-terminated string. On success,
// returns the string. Otherwise, returns NULL and sets the global 'errno'.
char *read_file(const char *filename);

#endif
