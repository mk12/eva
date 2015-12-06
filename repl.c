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

static char *saved_line = NULL;
static int saved_line_offset = 0;

void setup_readline(void) {
	rl_bind_key('\t', rl_insert);
}

void print_error(const char *err_msg) {
	fprintf(stderr, "ERROR: %s\n", err_msg);
}

struct ParseResult read_sexpr(void) {
	struct ParseResult result;
	result.chars_read = 0;
	result.err_msg = NULL;

	char *line;
	int length;
	struct ParseResult data;
	if (saved_line) {
		line = strdup(saved_line + saved_line_offset);
		free(saved_line);
		saved_line = NULL;
		saved_line_offset = 0;
		length = strlen(line);
		data = parse(line);
	} else {
		line = strdup("");
		length = 0;
		data.err_msg = err_unexpected_eoi;
	}

	while (data.err_msg == err_unexpected_eoi) {
		char *more = readline("");
		if (!more) {
			break;
		} else if (!*more) {
			free(more);
			continue;
		}
		add_history(more);
		int more_length = strlen(more);
		line = realloc(line, length + more_length + 2);
		line[length] = '\n';
		memcpy(line + length + 1, more, more_length);
		free(more);
		line[length + more_length + 1] = '\0';
		length += more_length + 1;
		data = parse(line);
	}

	if (!data.err_msg && data.chars_read < length) {
		saved_line = line;
		saved_line_offset = data.chars_read;
	} else {
		free(line);
	}

	return data;
}

void execute(const char *text, struct Environment *env, bool print) {
	struct EvalResult result;
	result.err_msg = NULL;

	int length = strlen(text);
	int read = 1;
	int offset = 0;
	while (read > 0 && offset < length) {
		struct ParseResult code = parse(text + offset);
		if (code.err_msg) {
			print_error(code.err_msg);
			break;
		}
		struct EvalResult result = eval_top(code.expr, env);
		if (result.err_msg) {
			print_error(result.err_msg);
			break;
		}
		if (print && offset + code.chars_read >= length) {
			print_expression(result.expr);
			putchar('\n');
		}
		release_expression(result.expr);
		release_expression(code.expr);
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
				if (code.err_msg == err_unexpected_eoi) {
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
					line = realloc(line, length + more_length + 2);
					line[length] = '\n';
					memcpy(line + length + 1, more, more_length);
					free(more);
					line[length + more_length + 1] = '\0';
					length += more_length + 1;
					continue;
				} else {
					print_error(code.err_msg);
					break;
				}
			} else {
				struct EvalResult result = eval_top(code.expr, env);
				if (result.err_msg) {
					print_error(result.err_msg);
				} else {
					print_expression(result.expr);
					putchar('\n');
					release_expression(result.expr);
				}
				release_expression(code.expr);
			}
			read = code.chars_read;
			offset += code.chars_read;
		}

		free(line);
	}
}
