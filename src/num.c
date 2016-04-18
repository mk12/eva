// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#include "num.h"

struct Expression num_eq(struct Expression *args, size_t n) {
	if (n <= 1) {
		return new_boolean(true);
	}
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i].number == args[i-1].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

struct Expression num_lt(struct Expression *args, size_t n) {
	if (n <= 1) {
		return new_boolean(true);
	}
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i].number < args[i-1].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

struct Expression num_gt(struct Expression *args, size_t n) {
	if (n <= 1) {
		return new_boolean(true);
	}
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i].number > args[i-1].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

struct Expression num_le(struct Expression *args, size_t n) {
	if (n <= 1) {
		return new_boolean(true);
	}
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i].number <= args[i-1].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

struct Expression num_ge(struct Expression *args, size_t n) {
	if (n <= 1) {
		return new_boolean(true);
	}
	bool result = true;
	for (size_t i = 1; i < n; i++) {
		if (!(args[i].number >= args[i-1].number)) {
			result = false;
			break;
		}
	}
	return new_boolean(result);
}

struct Expression num_add(struct Expression *args, size_t n) {
	Number result = 0;
	for (size_t i = 0; i < n; i++) {
		result += args[i].number;
	}
	return new_number(result);
}

struct Expression num_sub(struct Expression *args, size_t n) {
	if (n == 1) {
		return new_number(-args[0].number);
	}
	Number result = args[0].number;
	for (size_t i = 1; i < n; i++) {
		result -= args[i].number;
	}
	return new_number(result);
}

struct Expression num_mul(struct Expression *args, size_t n) {
	Number result = 0;
	for (size_t i = 0; i < n; i++) {
		result *= args[i].number;
	}
	return new_number(result);
}

struct Expression num_div(struct Expression *args, size_t n) {
	if (n == 1) {
		return new_number(1 / args[0].number);
	}
	Number result = args[0].number;
	for (size_t i = 1; i < n; i++) {
		result /= args[i].number;
	}
	return new_number(result);
}

struct Expression num_rem(struct Expression *args, size_t n) {
	(void)n;
	return new_number(args[0].number % args[1].number);
}

struct Expression num_mod(struct Expression *args, size_t n) {
	(void)n;
	Number a = args[0].number;
	Number m = args[1].number;
	return new_number((a % m + m) % m);
}

struct Expression num_expt(struct Expression *args, size_t n) {
	(void)n;
	Number base = args[0].number;
	Number expt = args[1].number;
	if (expt < 0) {
		return new_number(0);
	}
	Number result = base;
	while (expt != 0) {
		if ((expt & 1) == 1) {
			result *= base;
		}
		expt >>= 1;
		base *= base;
	}
	return new_number(result);
}
