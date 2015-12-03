// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "env.h"
#include "eval.h"
#include "parse.h"

static const char *prompt = "eva> ";

int main(void) {
	struct Environment *env = default_environment();
	setup_eval();

	for (;;) {
		// Read a line of input.
		char *str = readline(prompt);
		// Exit on the EOF condition.
		if (!str) {
			putchar('\n');
			break;
		}
		// Parse the string as an s-expression.
		struct ParseResult code = parse(str);
		if (code.err_msg) {
			fputs(code.err_msg, stderr);
		} else {
			// Evaluate the code.
			struct EvalResult result = eval_top(code.expr, env);
			if (result.err_msg) {
				fputs(result.err_msg, stderr);
			} else {
				// Print the resulting expression.
				print_expression(result.expr);
				release_expression(result.expr);
			}
			release_expression(code.expr);
		}

		putchar('\n');
		add_history(str);
		free(str);
	}
	return 0;
}
