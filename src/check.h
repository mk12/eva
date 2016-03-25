// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef CHECK_H
#define CHECK_H

static const char *check_application(
		struct Expression proc, struct Expression *args, size_t n);

static const char *check_list(struct Expression expr);

#endif
