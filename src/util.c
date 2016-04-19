// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "util.h"

#include <stdio.h>
#include <stdlib.h>

// Prints the errno message to standard error and exits with exit status 2.
static void die(void) __attribute__((noreturn)) {
	perror("FATAL");
	exit(2);
}

void *xmalloc(size_t size) {
	void *ptr = malloc(size);
	if (!ptr && size != 0) {
		die();
	}
	return ptr;
}

void *xcalloc(size_t count, size_t size) {
	void *ptr = calloc(count, size);
	if (!ptr && size != 0) {
		die();
	}
	return ptr;
}

void *xrealloc(void *ptr, size_t size) {
	ptr = realloc(ptr, size);
	if (!ptr && size != 0) {
		die();
	}
	return ptr;
}

char *read_file(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}
	long bufsize = ftell(file);
	if (bufsize == -1) {
		fclose(file);
		return NULL;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}
	char *buf = xmalloc((size_t)bufsize + 1);
	size_t length = fread(buf, 1, (size_t)bufsize, file);
	fclose(file);
	if (length == 0) {
		free(buf);
		return NULL;
	}
	buf[length+1] = '\0';
	return buf;
}
