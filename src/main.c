// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "env.h"
#include "error.h"
#include "eval.h"
#include "expr.h"
#include "repl.h"
#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#define isatty _isatty
#endif

// The usage message for the program.
static const char *const usage_message = "usage: eva [file ...] [-e code]\n";

// Error message used when an option argument is missing.
static const char *const err_opt_argument = "Option requires an argument";

// Processes the command line arguments. Returns true on success.
static bool process_args(int argc, char **argv, struct Environment *env) {
	if (argc == 2 && is_opt(argv[1], 'h', "help")) {
		fputs(usage_message, stdout);
		return true;
	}

	bool tty = isatty(0);
	if (argc == 1) {
		repl(env, tty);
		return true;
	}
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-") == 0) {
			repl(env, tty);
		} else if (is_opt(argv[i], 'e', "expression")) {
			// Execute the next argument.
			if (i == argc - 1) {
				print_error(argv[i], err_opt_argument);
				return false;
			}
			if (!execute(argv_filename, argv[++i], env, true)) {
				return false;
			}
		} else {
			// Assume the argument is a filename.
			char *text = read_file(argv[i]);
			if (!text) {
				print_file_error(argv[i]);
				return false;
			}
			bool success = execute(argv[i], text, env, false);
			free(text);
			if (!success) {
				return false;
			}
		}
	}
	return true;
}

int main(int argc, char **argv) {
	setup_readline();
	struct Environment *env = new_standard_environment();
	bool success = process_args(argc, argv, env);
	release_environment(env);
	return success ? 0 : 1;
}
