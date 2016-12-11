// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "repl.h"

#include "error.h"
#include "eval.h"
#include "parse.h"
#include "util.h"

#include <readline/readline.h>
#include <readline/history.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// String constants for prompts.
static const char *const primary_prompt = "eva> ";
static const char *const secondary_prompt = "...> ";

// Storage for buffers in between calls to 'read_sexpr'.
static char *saved_buffer = NULL;
static size_t saved_buffer_offset = 0;

void setup_readline(void) {
	// Disable tab completion.
	rl_bind_key('\t', rl_insert);
}

struct ParseError *read_sexpr(struct Expression *out) {
	char *buf;
	size_t buf_length;
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
		data.err_type = ERR_UNEXPECTED_EOI;
	}

	// Read lines until the string is parsed successfully, or until a parse
	// error is encountered that cannot be fixed by reading more input.
	while (data.err_type == ERR_UNEXPECTED_EOI) {
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
		size_t line_length = strlen(line);
		buf = xrealloc(buf, buf_length + line_length + 2);
		buf[buf_length] = '\n';
		memcpy(buf + buf_length + 1, line, line_length);
		free(line);
		buf[buf_length + line_length + 1] = '\0';
		buf_length += line_length + 1;
		// Attempt to parse the new buffer.
		data = parse(buf);
	}

	if (data.err_type != PARSE_SUCCESS) {
		// Give ownership of the buffer to the parse erorr.
		return new_parse_error((enum ParseErrorType)data.err_type,
				buf, data.chars_read);
	}

	// Save leftover input, if there is any.
	if (data.chars_read > 0 && data.chars_read < buf_length) {
		saved_buffer = buf;
		saved_buffer_offset = data.chars_read;
	} else {
		free(buf);
	}
	// Store the expression in the output location.
	*out = data.expr;
	return NULL;
}

// Returns the number of shebang characters at the beginning of 'text'. A
// shebang consists of "#!" followed by any characters until the end of the
// line, including the newline character.
static size_t skip_shebang(const char *text) {
	const char *s = text;
	if (*s++ != '#' || *s++ != '!') {
		return 0;
	}
	while (*s && *s != '\n') {
		s++;
	}
	if (*s) {
		s++;
	}
	return (size_t)(s - text);
}

bool execute(
		const char *filename,
		const char *text,
		struct Environment *env,
		bool print) {
	size_t length = strlen(text);
	size_t offset = skip_shebang(text);

	// Parse and evaluate until there is no text left.
	while (offset < length) {
		// Parse from the current offset.
		struct ParseResult code = parse(text + offset);
		if (code.err_type != PARSE_SUCCESS) {
			struct ParseError err = {
				.type = (enum ParseErrorType)code.err_type,
				.text = text,
				.index = offset + code.chars_read
			};
			print_parse_error(filename, &err);
			return false;
		}
		// Evaluate the expression.
		struct EvalResult result = eval(code.expr, env, true);
		if (result.err) {
			print_eval_error(filename, result.err);
			free_eval_error(result.err);
			release_expression(code.expr);
			return false;
		}
		// Print the result if it is not void.
		if (print && result.expr.type != E_VOID) {
			print_expression(result.expr, stdout);
			putchar('\n');
		}
		release_expression(result.expr);
		release_expression(code.expr);
		offset += code.chars_read;
	}
	return true;
}

void repl(struct Environment *env, bool interactive) {
	const char *prompt1 = interactive ? primary_prompt : "";
	const char *prompt2 = interactive ? secondary_prompt : "";
	bool shebang = !interactive;

	for (;;) {
		char *buf = readline(prompt1);
		if (!buf) {
			// EOF: stop the loop.
			if (interactive) {
				putchar('\n');
			}
			return;
		}
		if (*buf) {
			// Add to the GNU Readline history if necessary.
			if (interactive) {
				add_history(buf);
			}
		} else {
			// Empty string: ignore it.
			free(buf);
			continue;
		}

		// Skip the shebang on the first line of non-interactive sessions.
		size_t buf_length = strlen(buf);
		size_t offset = shebang ? skip_shebang(buf) : 0;
		shebang = false;

		// Parse and evaluate until there is no text left.
		while (offset < buf_length) {
			// Parse from the current offset.
			struct ParseResult code = parse(buf + offset);
			if (code.err_type == PARSE_SUCCESS) {
				// Evaluate the expression.
				struct EvalResult result = eval(code.expr, env, true);
				if (result.err) {
					print_eval_error(stdin_filename, result.err);
					free_eval_error(result.err);
					release_expression(code.expr);
					if (interactive) {
						break;
					} else {
						free(buf);
						return;
					}
				}
				if (result.expr.type != E_VOID) {
					print_expression(result.expr, stdout);
					putchar('\n');
				}
				release_expression(result.expr);
				release_expression(code.expr);
				offset += code.chars_read;
			} else {
				if (code.err_type != ERR_UNEXPECTED_EOI) {
					struct ParseError err = {
						.type = (enum ParseErrorType)code.err_type,
						.text = buf,
						.index = offset + code.chars_read
					};
					print_parse_error(stdin_filename, &err);
					if (interactive) {
						break;
					} else {
						free(buf);
						return;
					}
				}
				// Read another line of input.
				char *line = readline(prompt2);
				if (!line) {
					// EOF: stop the loop.
					free(buf);
					if (interactive) {
						putchar('\n');
					}
					return;
				}
				if (!*line) {
					// Empty string: ignore it.
					free(line);
					continue;
				}
				if (interactive) {
					// Add to the GNU Readline history.
					add_history(line);
				}
				size_t line_length = strlen(line);
				// Concatenate the line to the end of the buffer.
				buf = xrealloc(buf, buf_length + line_length + 2);
				buf[buf_length] = '\n';
				memcpy(buf + buf_length + 1, line, line_length);
				free(line);
				buf[buf_length + line_length + 1] = '\0';
				buf_length += line_length + 1;
			}
		}

		free(buf);
	}
}
