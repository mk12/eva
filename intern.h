// Copyright 2015 Mitchell Kember. Subject to the MIT License.

#ifndef INTERN_H
#define INTERN_H

// Interns the string and returns its unique identifier.
int intern_string(const char *str);

// Interns a string of n characters. Does not require a null terminator.
int intern_string_n(const char *str, int n);

// Looks up a string by identifier.
const char *find_string(int id);

#endif
