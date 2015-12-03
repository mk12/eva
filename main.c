// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "env.h"
#include "eval.h"
#include "parse.h"

int main(void) {
	struct Environment *env = default_environment();
	setup_eval();

	for (;;) {
		char *str = readline("eva> ");
		if (!str) {
			putchar('\n');
			break;
		}
		struct ParseResult pres = parse(str);
		if (pres.err_msg) {
			fputs(pres.err_msg, stderr);
		} else {
			struct EvalResult eres = eval(pres.expr, env);
			if (eres.err_msg) {
				fputs(eres.err_msg, stderr);
			} else {
				print_expression(eres.expr);
				release_expression(eres.expr);
			}
			release_expression(pres.expr);
		}
		putchar('\n');
		add_history(str);
		free(str);
	}
	return 0;
}
