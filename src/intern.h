// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef INTERN_H
#define INTERN_H

#include <stddef.h>

typedef unsigned int InternID;

// Interns the string and returns its unique identifier.
InternID intern_string(const char *str);

// Interns a string of n characters. Does not require a null terminator. If the
// same string has previously been interned, returns the same identifier.
InternID intern_string_n(const char *str, size_t n);

// Looks up a string by identifier.
const char *find_string(InternID id);

#endif
