#ifndef TG_STRING_H
#define TG_STRING_H

#include "tg_common.h"
#include "tg_variadic.h"

u32            tg_string_copy(u32 size, char* p_buffer, const char* p_string);
u32            tg_string_copy_no_nul(u32 size, char* p_buffer, const char* p_string);
b32            tg_string_equal(const char* p_s0, const char* p_s1);

/*
%c - char
%d - f64
%i - i32
%s - string
%u - u32
*/
void           tg_string_format(u32 size, char* p_buffer, const char* p_format, ...);
void           tg_string_format_va(u32 size, char* p_buffer, const char* p_format, char* p_variadic_arguments);
u32            tg_string_length(const char* p_string);
u32            tg_string_parse_i32(u32 size, char* p_buffer, i32 v);
u32            tg_string_parse_i32_no_nul(u32 size, char* p_buffer, i32 v);
void           tg_string_replace_characters(char* p_string, char replace, char with);
f32            tg_string_to_f32(const char* p_string);
i32            tg_string_to_i32(const char* p_string);

#endif
