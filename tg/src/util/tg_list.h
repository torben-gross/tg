#ifndef TG_LIST_H
#define TG_LIST_H

#include "tg_common.h"

#define    TG_LIST_DEFAULT_CAPACITY                                          32

#define    TG_LIST_CREATE(type)                                              tg_list_create_impl(sizeof(type), TG_LIST_DEFAULT_CAPACITY);
#define    TG_LIST_CREATE__CAPACITY(type, capacity)                          tg_list_create_impl(sizeof(type), capacity);
#define    TG_LIST_CREATE__ELEMENT_SIZE(element_size)                        tg_list_create_impl(element_size, TG_LIST_DEFAULT_CAPACITY);
#define    TG_LIST_CREATE__ELEMENT_SIZE__CAPACITY(element_size, capacity)    tg_list_create_impl(element_size, capacity);



TG_DECLARE_HANDLE(tg_list);



tg_list_h    tg_list_create_impl(u32 element_size, u32 capacity);
void         tg_list_destroy(tg_list_h list_h);

u32          tg_list_capacity(const tg_list_h list_h);
b32          tg_list_contains(const tg_list_h list_h, const void* p_value);
u32          tg_list_count(const tg_list_h list_h);
void         tg_list_insert(tg_list_h list_h, const void* p_value);
void         tg_list_insert_at(tg_list_h list_h, u32 index, const void* p_value);
void         tg_list_insert_at_unchecked(tg_list_h list_h, u32 index, const void* p_value);
void         tg_list_insert_list(tg_list_h list0_h, const tg_list_h list1_h);
void         tg_list_insert_unchecked(tg_list_h list_h, const void* p_value);
void*        tg_list_pointer_to(tg_list_h list_h, u32 index);
void         tg_list_remove(tg_list_h list_h, const void* p_value);
void         tg_list_remove_at(tg_list_h list_h, u32 index);
void         tg_list_replace_at(tg_list_h list_h, u32 index, const void* p_value);
void         tg_list_reserve(tg_list_h list_h, u32 capacity);

#endif
