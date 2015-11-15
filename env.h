// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef ENV_H
#define ENV_H

struct Environment;
struct Expression;

// Looks up a variable in the environment by its identifier. Returns NULL if it
// doesn't appear.
struct Expression *lookup(struct Environment *env, int id);

// Binds n variables to their respective expressions in the environment. Returns
// the augmented environment.
struct Environment *bind(
		struct Environment *env,
		int *vars,
		struct Expression **vals,
		int n);

// Unbinds the n most recently bound variables. Returns the reduced environment.
// Does not deallocate variables or expressions.
struct Environment *unbind(struct Environment *env, int n);

// Returns an environment containing mappings for built-in special functions.
struct Environment *default_environment(void);

#endif
