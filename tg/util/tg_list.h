#ifndef TG_LIST_H
#define TG_LIST_H

#include "tg/tg_common.h"

#define    TG_LIST_DEFAULT_CAPACITY 32

#define    tg_list_create(         type,           p_list_h)    tg_list_create_impl(TG_LIST_DEFAULT_CAPACITY, sizeof(type), p_list_h);
#define    tg_list_create_capacity(type, capacity, p_list_h)    tg_list_create_impl(                capacity, sizeof(type), p_list_h);



typedef struct tg_list tg_list;
typedef tg_list* tg_list_h;



void     tg_list_create_impl(u32 capacity, u32 element_size, tg_list_h* p_list_h);
void     tg_list_reserve(tg_list_h list_h, u32 capacity);
void     tg_list_insert(tg_list_h list_h, const void* p_value);
void     tg_list_insert_unchecked(tg_list_h list_h, const void* p_value);
void     tg_list_insert_at(tg_list_h list_h, u32 index, const void* p_value);
void     tg_list_insert_at_unchecked(tg_list_h list_h, u32 index, const void* p_value);
void     tg_list_replace_at(tg_list_h list_h, u32 index, const void* p_value);
void*    tg_list_at(tg_list_h list_h, u32 index);
void     tg_list_destroy(tg_list_h list_h);

#endif
