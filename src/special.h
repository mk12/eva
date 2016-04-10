// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef SPECIAL_H
#define SPECIAL_H

// Special forms are the core of the Eva language. They are the syntactical
// forms that require special evaluation rules.
#define N_SPECIAL_FORMS 10
enum SpecialForm {
	// Basic syntax:
	F_DEFINE,
	F_LAMBDA,
	F_QUOTE,
	F_BEGIN,
	// Conditionals:
	F_COND,
	F_IF,
	// Let bindings:
	F_LET,
	F_LET_STAR,
	// Logical operators:
	F_AND,
	F_OR
};

// This should be called once at the beginning of the program.
void setup_special(void);

// Looks up a special form by intern identifier. If it exists, returns the
// SpecialForm enum value. Otherwise, returns -1.
static int find_special_form(InternId id);

// Returns true if the special form will accept 'n' parts in its body. Returns
// false if this would be a syntax error.
bool accepts_n_parts(enum SpecialForm special, size_t n);

#endif
