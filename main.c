// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#include <stdio.h>

#include "eval.h"

int main(void) {
	setup_eval();
	return 0;
}

// TODO:
// becuase of eval/apply recursion, env define logic can't be pulled out
// instead expose eval-block which takes seq of expressions and maintains the
// array itself.
//
// Releasing expresssions:
// code tree is +1 when parsed; after evaluating, release it.
//
// But REPL evaluates one by one ...
//
//
//
//
// The body of the procedure is not the single expression, it is the list of
// expressions.
// - add "(begin" and then implement begin blocks
// - OR don't allow block structure ?
// - need same thing in cond
// - need evaluate that takes (e1 e2 e3) and evaluates each one at a time
// - then "(begin" can call this on the cdr, easy.
//
// Env should be a big hash table.
