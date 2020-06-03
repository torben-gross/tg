#ifndef TG_STRING_H
#define TG_STRING_H

#include "tg_common.h"
#include <stdarg.h>

b32     tg_string_equal(const char* p_s0, const char* p_s1);
/*
%d - f64
%i - i32
%s - string
%u - u32
*/
void    tg_string_format(u32 size, char* p_buffer, const char* p_format, ...);
void    tg_string_format_va(u32 size, char* p_buffer, const char* p_format, va_list v);
u32     tg_string_length(const char* p_string);

#endif
