// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "env.h"
#include "eval.h"
#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#define isatty _isatty
#endif

static const char *err_open_file = "can't open file";
static const char *err_read_file = "can't read file";
static const char *err_no_expr = "expected expression after -e";

struct ReadResult {
	char *buf;
	const char *err_msg;
};

// Reads the entire contents of a file into a buffer.
static struct ReadResult read_file(const char *filename) {
	struct ReadResult result;
	result.err_msg = NULL;

	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		result.err_msg = err_open_file;
		return result;
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		result.err_msg = err_open_file;
		return result;
	}
	long bufsize = ftell(file);
	if (bufsize == -1) {
		fclose(file);
		result.err_msg = err_open_file;
		return result;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		result.err_msg = err_open_file;
		return result;
	}
	result.buf = malloc((size_t)bufsize + 1);
	size_t length = fread(result.buf, 1, (size_t)bufsize, file);
	fclose(file);
	if (length == 0) {
		free(result.buf);
		result.err_msg = err_read_file;
		return result;
	}
	result.buf[length+1] = '\0';
	return result;
}

int main(int argc, char **argv) {
	// Print the usage message if passed the -h flag.
	if (argc == 2 &&
			(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		puts("usage: eva [file] [-e code]");
		return 0;
	}

	// Set up the environment and evaluator.
	struct Environment *env = default_environment();
	setup_readline();
	setup_eval();

	// Loop over all the command-line arguments..
	bool interactive = false;
	bool error = false;
	for (int i = 1; i < argc; i++) {
		// Check for the interactive flag.
		if (strcmp(argv[i], "-") == 0
				|| strcmp(argv[i], "-i") == 0
				|| strcmp(argv[i], "--interactive") == 0) {
			interactive = true;
			continue;
		}
		// Check for inline expression commands.
		if (strcmp(argv[i], "-e") == 0) {
			i++;
			if (i == argc) {
				fprintf(stderr, "%s%s\n", error_prefix, err_no_expr);
				error = true;
				break;
			}
			// Execute the expression.
			if (!execute(argv[i], env, true)) {
				error = true;
				break;
			}
			continue;
		}
		// Assume the argument is a filename.
		struct ReadResult result = read_file(argv[i]);
		if (result.err_msg) {
			fprintf(stderr, "%s%s '%s'\n",
				error_prefix, result.err_msg, argv[i]);
			error = true;
			break;
		}
		// Execute the contents of the file.
		bool success = execute(result.buf, env, false);
		free(result.buf);
		if (!success) {
			error = true;
			break;
		}
	}
	if (error) {
		free_environment(env);
		return 1;
	}

	// Start the REPL if interactive mode was specified, or if no arguments were
	// provided, or if input is coming from a pipe.
	bool tty = isatty(0);
	if (interactive || argc == 1 || !tty) {
		repl(env, tty);
	}

	// Clean up and exit.
	free_environment(env);
	return 0;
}
