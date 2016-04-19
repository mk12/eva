// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Prints the errno message to standard error and exits with exit status 2.
static void die(void) __attribute__((noreturn));
static void die(void) {
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

bool is_opt(const char *arg, char short_opt, const char *long_opt) {
	size_t len = strlen(arg);
	return len >= 2 && arg[0] == '-'
		&& ((arg[1] == short_opt && arg[2] == '\0')
				|| (arg[1] == '-' && strcmp(arg + 2, long_opt) == 0));
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
