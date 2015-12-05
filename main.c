// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include "env.h"
#include "eval.h"
#include "repl.h"

int main(void) {
	struct Environment *env = default_environment();
	setup_readline();
	setup_eval();
	repl(env);
	free_environment(env);
	return 0;
}
