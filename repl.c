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

void execute(const char *text, struct Environment *env, bool print) {
	struct EvalResult result;
	result.err_msg = NULL;

	int length = strlen(text);
	int read = 1;
	int offset = 0;
	while (read > 0 && offset < length) {
		struct ParseResult code = parse(text + offset);
		if (code.err_msg) {
			fputs(code.err_msg, stderr);
			putchar('\n');
			break;
		} else {
			struct EvalResult result = eval_top(code.expr, env);
			if (result.err_msg) {
				fputs(result.err_msg, stderr);
				putchar('\n');
			} else {
				if (print && offset + code.chars_read >= length) {
					print_expression(result.expr);
					putchar('\n');
				}
				release_expression(result.expr);
			}
			release_expression(code.expr);
		}
		read = code.chars_read;
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
		if (*line) {
			add_history(line);
		} else {
			free(line);
			continue;
		}

		int length = strlen(line);
		int read = 1;
		int offset = 0;
		while (read > 0 && offset < length) {
			struct ParseResult code = parse(line + offset);
			if (code.err_msg) {
				if (more_input(code.err_msg)) {
					char *more = readline(secondary_prompt);
					if (!more) {
						free(line);
						putchar('\n');
						return;
					}
					if (!*more) {
						free(more);
						continue;
					}
					add_history(more);
					int more_length = strlen(more);
					char *combined = realloc(line, length + more_length + 2);
					combined[length] = '\n';
					memcpy(combined + length + 1, more, more_length);
					combined[length + more_length + 1] = '\0';
					length += more_length + 1;
					continue;
				} else {
					fputs(code.err_msg, stderr);
					putchar('\n');
				}
			} else {
				struct EvalResult result = eval_top(code.expr, env);
				if (result.err_msg) {
					fputs(result.err_msg, stderr);
					putchar('\n');
				} else {
					print_expression(result.expr);
					release_expression(result.expr);
					putchar('\n');
				}
				release_expression(code.expr);
			}
			read = code.chars_read;
			offset += code.chars_read;
		}
		free(line);
	}
}
