#ifndef TG_STRING_H
#define TG_STRING_H

#include "tg/tg_common.h"

/*
%d - f64
%i - i32
%s - string
%u - u32
*/
void    tg_string_format(u32 size, char* p_buffer, const char* p_format, ...);
u32     tg_string_length(const char* p_string);

#endif
