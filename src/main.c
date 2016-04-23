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

int main(int argc, char **argv) {
	// Check for the help flag.
	if (argc == 2 && is_opt(argv[1], 'h', "help")) {
		fputs(usage_message, stdout);
		return 0;
	}

	setup_readline();
	struct Environment *env = new_standard_environment();
	bool interactive = false;
	bool error = false;
	for (int i = 1; i < argc; i++) {
		// Check for the interactive flag.
		if (strcmp(argv[i], "-") == 0 || is_opt(argv[i], 'i', "interactive")) {
			interactive = true;
			continue;
		}
		// Check for inline expressions to evaluate.
		if (is_opt(argv[i], 'e', "expression")) {
			i++;
			if (i == argc) {
				fputs(usage_message, stderr);
				error = true;
				break;
			}
			if (!execute(argv_filename, argv[i], env, true)) {
				error = true;
				break;
			}
			continue;
		}
		// Assume the argument is a filename.
		char *text = read_file(argv[i]);
		if (!text) {
			print_file_error(argv[i]);
			error = true;
			break;
		}
		bool success = execute(argv[i], text, env, false);
		free(text);
		if (!success) {
			error = true;
			break;
		}
	}
	if (error) {
		release_environment(env);
		return 1;
	}

	// Start the REPL if interactive mode was requested, or the program was
	// called with no arguments, or input is coming from a pipe (not a tty).
	bool tty = isatty(0);
	if (interactive || argc == 1 || !tty) {
		repl(env, tty);
	}

	release_environment(env);
	return 0;
}
