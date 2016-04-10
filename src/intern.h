// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef INTERN_H
#define INTERN_H

#include <stddef.h>
#include <stdint.h>

// An InternId is a unique identifier for an interned string.
typedef uint32_t InternId;

// Interns the string and returns its unique identifier.
InternId intern_string(const char *str);

// Interns a string of 'n' characters. Does not require a null terminator. If
// the same string has previously been interned, returns the same identifier.
InternId intern_string_n(const char *str, size_t n);

// Looks up a string by identifier. The identifier 'id' must be a value returned
// from 'intern_string' or 'intern_string_n' (this is not checked).
const char *find_string(InternId id);

// Each interned string has one byte of metadata associated with it. By default,
// the metadata byte is zero. These functions modify and retrieve it. As with
// 'find_string', the identifier 'id' must be valid.
void set_metadata(InternId id, uint8_t metadata);
uint8_t find_metadata(InternId id);

#endif
