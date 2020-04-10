#ifndef TG_LIST_H
#define TG_LIST_H

#include "tg_common.h"

#define    TG_LIST_DEFAULT_CAPACITY                                   32

#define    tg_list_create(         type,           p_list_h)          tg_list_create_impl(TG_LIST_DEFAULT_CAPACITY, sizeof(type), p_list_h);
#define    tg_list_create_capacity(type, capacity, p_list_h)          tg_list_create_impl(                capacity, sizeof(type), p_list_h);
#define    tg_list_create_capacity_size( capacity, size, p_list_h)    tg_list_create_impl(                capacity,         size, p_list_h);
#define    tg_list_create_size(    type,           size, p_list_h)    tg_list_create_impl(TG_LIST_DEFAULT_CAPACITY,         size, p_list_h);



TG_DECLARE_HANDLE(tg_list);



void     tg_list_create_impl(u32 capacity, u32 element_size, tg_list_h* p_list_h);
void     tg_list_destroy(tg_list_h list_h);

u32      tg_list_capacity(const tg_list_h list_h);
b32      tg_list_contains(const tg_list_h list_h, const void* p_value);
u32      tg_list_count(const tg_list_h list_h);
void     tg_list_insert(tg_list_h list_h, const void* p_value);
void     tg_list_insert_at(tg_list_h list_h, u32 index, const void* p_value);
void     tg_list_insert_at_unchecked(tg_list_h list_h, u32 index, const void* p_value);
void     tg_list_insert_unchecked(tg_list_h list_h, const void* p_value);
void*    tg_list_pointer_to(tg_list_h list_h, u32 index);
void     tg_list_remove(tg_list_h list_h, const void* p_value);
void     tg_list_remove_at(tg_list_h list_h, u32 index);
void     tg_list_replace_at(tg_list_h list_h, u32 index, const void* p_value);
void     tg_list_reserve(tg_list_h list_h, u32 capacity);

#endif
