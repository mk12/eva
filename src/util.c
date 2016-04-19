// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "util.h"

#include "error.h"

#include <stdio.h>
#include <stdlib.h>

// Prints the errno message to standard error and exits with exit status 2.
static void die(void) {
	perror("FATAL");
	exit(2);
}

void *safe_malloc(size_t size) {
	void *ptr = malloc(size);
	if (!ptr && size != 0) {
		die();
	}
	return ptr;
}

void *safe_calloc(size_t size) {
	void *ptr = calloc(1, size);
	if (!ptr && size != 0) {
		die();
	}
	return ptr;
}

void *safe_realloc(void *ptr, size_t size) {
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
	char *buf = malloc((size_t)bufsize + 1);
	size_t length = fread(result.buf, 1, (size_t)bufsize, file);
	fclose(file);
	if (length == 0) {
		free(buf);
		return NULL;
	}
	buf[length+1] = '\0';
	return buf;
}
