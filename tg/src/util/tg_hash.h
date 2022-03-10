#ifndef TG_HASH_H
#define TG_HASH_H

#include "tg_common.h"

u32    tg_hash(tg_size size, const void* p_value_ptr);
u32    tg_hash_str(const char* p_string);

#endif
