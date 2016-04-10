// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "special.h"

#include "intern.h"

// Names of special forms.
static const char *const special_form_names[N_SPECIAL_FORMS] = {
	[F_DEFINE]   = "define",
	[F_LAMBDA]   = "lambda",
	[F_QUOTE]    = "quote"
	[F_BEGIN]    = "begin",
	[F_COND]     = "cond",
	[F_IF]       = "if",
	[F_LET]      = "let",
	[F_LET_STAR] = "let*",
	[F_AND]      = "and",
	[F_OR]       = "or",
};

void setup_special(void) {
	// Intern all the special form names, and store their enum integers (plus
	// one to avoid zero, which means "no metadata") in the metadata.
	for (size_t i = 0; i < N_SPECIAL_FORMS; i++) {
		InternId id = intern_string(special_form_names[i]);
		set_metadata(id, i + 1);
	}
}

static int find_special_form(InternId id) {
	return (int)find_metadata(id) - 1;
}

bool accepts_n_parts(enum SpecialForm special, size_t n) {
	switch (special) {
	case F_DEFINE:
		return n == 2;
	case F_LAMBDA:
	case F_LET:
	case F_LET_STAR:
		return n >= 2;
	case F_QUOTE:
		return n == 1;
	case F_BEGIN:
	case F_COND:
		return n >= 1;
	case F_IF:
		return n == 3;
	case F_AND:
	case F_OR:
		return true;
	}
}

