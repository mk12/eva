// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "eval.h"
#include "parse.h"

static const char *primary_prompt = "eva> ";
static const char *secondary_prompt = "...> ";

void EvalResult execute(
		const char *text, struct Environment *env, bool print) {
	struct EvalResult result;
	result.err_msg = NULL;

	int length = strlen(text);
	int offset = 0;
	while (offset < length) {
		struct ParseResult code = parse(text + offset, NULL);
		if (code.err_msg) {
			fputs(code.err_msg, stderr);
			break;
		} else {
			struct EvalResult result = eval_top(code.expr, env);
			if (result.err_msg) {
				fputs(result.err_msg, stderr);
			} else {
				if (print && offset + code.chars_read >= length) {
					print_expression(result.expr);
					putchar('\n');
				}
				release_expression(result.expr);
			}
			release_expression(code.expr);
		}
		offset += code.chars_read;
	}
}

void repl(struct Environment *env) {
	for (;;) {
		char *line = readline(primary_prompt);
		if (!line) {
			putchar('\n');
			return;
		}
		add_history(line);

		int length = strlen(line);
		int offset = 0;
		while (offset < length) {
			struct ParseResult code = parse(line + offset, read_more);
			if (code.err_msg) {
				if (more_input(code.err_msg)) {
					char *more = readline(secondary_prompt);
					if (!more) {
						free(line);
						putchar('\n');
						return;
					}
					add_history(more);
					int more_length = strlen(more);
					char *combined = realloc(line, length + more_length + 1);
					memcpy(combined + length, more, more_length);
					combined[length + more_length] = '\0';
					length += more_length;
					continue;
				} else {
					fputs(code.err_msg, stderr);
				}
			} else {
				struct EvalResult result = eval_top(code.expr, env);
				if (result.err_msg) {
					fputs(result.err_msg, stderr);
				} else {
					print_expression(result.expr);
					release_expression(result.expr);
					putchar('\n');
				}
				release_expression(code.expr);
			}
			offset += code.chars_read;
		}
		free(line);
	}
}
