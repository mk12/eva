// Copyright 2016 Mitchell Kember. Subject to the MIT License.

#ifndef ERROR_H
#define ERROR_H

// enum {
// 	ERR_OP_NOT_PROC,
// 	ERR_
// };
//
// extern const char errors[

extern const char *err_op_not_proc;
extern const char *err_arity;
extern const char *err_not_num;
extern const char *err_not_bool;
extern const char *err_not_proc;
extern const char *err_not_list;
extern const char *err_not_pair;
extern const char *err_unbound_var;
extern const char *err_ill_list;
extern const char *err_ill_define;
extern const char *err_ill_placed_define;
extern const char *err_ill_quote;
extern const char *err_ill_if;
extern const char *err_ill_cond;
extern const char *err_ill_lambda;
extern const char *err_ill_let;
extern const char *err_ill_begin;
extern const char *err_divide_zero;
extern const char *err_dup_param;
extern const char *err_special_var;
extern const char *err_non_exhaustive;

#endif
