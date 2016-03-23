// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "eval.h"
#include "parse.h"

const char *error_prefix = "ERROR: ";

static const char *primary_prompt = "eva> ";
static const char *secondary_prompt = "...> ";

static char *saved_buffer = NULL;
static int saved_buffer_offset = 0;

void setup_readline(void) {
	// Disable tab completion.
	rl_bind_key('\t', rl_insert);
}

static void print_error(const char *err_msg) {
	fprintf(stderr, "%s%s\n", error_prefix, err_msg);
}

struct ParseResult read_sexpr(void) {
	struct ParseResult result;
	result.chars_read = 0;
	result.err_msg = NULL;

	char *buf;
	int buf_length;
	struct ParseResult data;
	if (saved_buffer) {
		// If there is leftover input, use it.
		buf = strdup(saved_buffer + saved_buffer_offset);
		free(saved_buffer);
		saved_buffer = NULL;
		saved_buffer_offset = 0;
		buf_length = strlen(buf);
		data = parse(buf);
	} else {
		// Otherwise, start with an empty string.
		buf = strdup("");
		buf_length = 0;
		data.err_msg = err_unexpected_eoi;
	}

	// Read lines until the string is parsed successfully, or until a parse
	// error is encountered that cannot be fixed by reading more input.
	while (data.err_msg == err_unexpected_eoi) {
		char *line = readline("");
		if (!line) {
			// EOF: stop reading.
			break;
		} else if (!*line) {
			// Empty string: keep reading.
			free(line);
			continue;
		}
		// Add to the GNU Readline history.
		add_history(line);
		// Concatenate the line to the end of the buffer.
		int line_length = strlen(line);
		buf = realloc(buf, buf_length + line_length + 2);
		buf[buf_length] = '\n';
		memcpy(buf + buf_length + 1, line, line_length);
		free(line);
		buf[buf_length + line_length + 1] = '\0';
		buf_length += line_length + 1;
		// Attempt to parse the new buffer.
		data = parse(buf);
	}

	// Save any leftover input.
	if (!data.err_msg && data.chars_read > 0 && data.chars_read < buf_length) {
		saved_buffer = buf;
		saved_buffer_offset = data.chars_read;
	} else {
		free(buf);
	}

	return data;
}

bool execute(const char *text, struct Environment *env, bool print) {
	struct EvalResult result;
	result.err_msg = NULL;
	int length = strlen(text);
	int offset = 0;

	// Parse and evaluate until there is no text left.
	while (offset < length) {
		// Parse from the current offset.
		struct ParseResult code = parse(text + offset);
		if (code.err_msg) {
			print_error(code.err_msg);
			return false;
		}
		// Evaluate the expression.
		struct EvalResult result = eval_top(code.expr, env);
		if (result.err_msg) {
			print_error(result.err_msg);
			release_expression(code.expr);
			return false;
		}
		// If print is true and this is the last expression, print it.
		if (print && offset + code.chars_read >= length) {
			print_expression(result.expr);
			putchar('\n');
		}
		release_expression(result.expr);
		release_expression(code.expr);
		offset += code.chars_read;
	}
	return true;
}

void repl(struct Environment *env, bool print) {
	for (;;) {
		char *buf = readline(print ? primary_prompt : "");
		if (!buf) {
			// EOF: stop the loop.
			if (print) {
				putchar('\n');
			}
			return;
		}
		if (*buf) {
			// Add to the GNU Readline history if necessary.
			if (print) {
				add_history(buf);
			}
		} else {
			// Empty string: ignore it.
			free(buf);
			continue;
		}

		int buf_length = strlen(buf);
		int offset = 0;
		// Parse and evaluate until there is no text left.
		while (offset < buf_length) {
			// Parse from the current offset.
			struct ParseResult code = parse(buf + offset);
			if (code.err_msg) {
				if (code.err_msg == err_unexpected_eoi) {
					// Read another line of input.
					char *line = readline(print ? secondary_prompt : "");
					if (!line) {
						// EOF: stop the loop.
						free(buf);
						putchar('\n');
						return;
					}
					if (!*line) {
						// Empty string: ignore it.
						free(line);
						continue;
					}
					if (print) {
						// Add to the GNU Readline history.
						add_history(line);
					}
					int line_length = strlen(line);
					// Concatenate the line to the end of the buffer.
					buf = realloc(buf, buf_length + line_length + 2);
					buf[buf_length] = '\n';
					memcpy(buf + buf_length + 1, line, line_length);
					free(line);
					buf[buf_length + line_length + 1] = '\0';
					buf_length += line_length + 1;
					continue;
				} else {
					print_error(code.err_msg);
					break;
				}
			} else {
				// Evaluate the expression.
				struct EvalResult result = eval_top(code.expr, env);
				if (result.err_msg) {
					print_error(result.err_msg);
				} else {
					if (print) {
						print_expression(result.expr);
						putchar('\n');
					}
					release_expression(result.expr);
				}
				release_expression(code.expr);
			}
			offset += code.chars_read;
		}

		free(buf);
	}
}
