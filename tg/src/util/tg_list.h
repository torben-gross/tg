#ifndef TG_LIST_H
#define TG_LIST_H

#include "tg_common.h"

#define TG_LIST_DEFAULT_CAPACITY    32
#define TG_LIST_AT(list, index)     ((void*)&(list).p_elements[(index) * (list).element_size])



typedef void tg_destroy_fn(void* p_element);
typedef struct tg_list
{
	tg_size           element_size;
	u32               capacity;
	u32               count;
	tg_destroy_fn*    p_destroy_fn;
	u8* p_elements;
} tg_list;



tg_list    tg_list_create(tg_size element_size, u32 capacity, tg_destroy_fn* p_destroy_fn);
void       tg_list_destroy(tg_list* p_list);

void       tg_list_clear(tg_list* p_list);
b32        tg_list_contains(const tg_list* p_list, const void* p_value);
void       tg_list_insert(tg_list* p_list, const void* p_value);
void       tg_list_insert_at(tg_list* p_list, u32 index, const void* p_value);
void       tg_list_insert_at_unchecked(tg_list* p_list, u32 index, const void* p_value);
void       tg_list_insert_list(tg_list* p_list0, tg_list* p_list1);
void       tg_list_insert_unchecked(tg_list* p_list, const void* p_value);
void       tg_list_remove(tg_list* p_list, const void* p_value);
void       tg_list_remove_at(tg_list* p_list, u32 index);
void       tg_list_replace_at(tg_list* p_list, u32 index, const void* p_value);
void       tg_list_reserve(tg_list* p_list, u32 capacity);

#endif
