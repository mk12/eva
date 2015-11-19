// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef INTERN_H
#define INTERN_H

typedef unsigned int InternID;

// Interns the string and returns its unique identifier.
InternID intern_string(const char *str);

// Interns a string of n characters. Does not require a null terminator.
InternID intern_string_n(const char *str, int n);

// Looks up a string by identifier.
const char *find_string(InternId id);

#endif
