#include "util/tg_list.h"

#include "memory/tg_memory.h"
#include <string.h> // TODO: memcpy


#define TG_LIST_SET_AT(list, index, p_value)    memcpy(TG_LIST_POINTER_TO(list, index), p_value, (list).element_size)



tg_list tg_list_create_impl(u32 element_size, u32 capacity)
{
	TG_ASSERT(element_size && capacity);

	tg_list list = { 0 };
	list.element_size = element_size;
	list.capacity = capacity;
	list.count = 0;
	list.elements = TG_MEMORY_ALLOC((u64)capacity * (u64)element_size);

	return list;
}

void tg_list_destroy(tg_list* p_list)
{
	TG_ASSERT(p_list);

	TG_MEMORY_FREE(p_list->elements);
}



b32 tg_list_contains(const tg_list* p_list, const void* p_value)
{
	TG_ASSERT(p_list && p_value);

	b32 contains = TG_FALSE;
	for (u32 i = 0; i < p_list->count; i++)
	{
		b32 equal = TG_TRUE;
		for (u32 b = 0; b < p_list->element_size; b++)
		{
			equal |= p_list->elements[i * p_list->element_size + b] == ((u8*)p_value)[b];
		}
		contains &= equal;
	}
	return contains;
}

void tg_list_insert(tg_list* p_list, const void* p_value)
{
	TG_ASSERT(p_list && p_value);

	if (p_list->capacity == p_list->count)
	{
		const u32 new_capacity = 2 * p_list->capacity;
		tg_list_reserve(p_list, new_capacity);
	}

	tg_list_insert_unchecked(p_list, p_value);
}

void tg_list_insert_at(tg_list* p_list, u32 index, const void* p_value)
{
	TG_ASSERT(p_list && index <= p_list->count && p_value);

	if (p_list->capacity == p_list->count)
	{
		const u32 new_capacity = 2 * p_list->capacity;
		tg_list_reserve(p_list, new_capacity);
	}

	tg_list_insert_at_unchecked(p_list, index, p_value);
}

void tg_list_insert_at_unchecked(tg_list* p_list, u32 index, const void* p_value)
{
	TG_ASSERT(p_list && index <= p_list->count && p_list->capacity > p_list->count && p_value);

	for (u32 i = p_list->count - 1; i >= index; i--)
	{
		TG_LIST_SET_AT(*p_list, i + 1, TG_LIST_POINTER_TO(*p_list, i));
	}
	TG_LIST_SET_AT(*p_list, index, p_value);
	p_list->count++;
}

void tg_list_insert_list(tg_list* p_list0, tg_list* p_list1)
{
	TG_ASSERT(p_list0 && p_list1);

	for (u32 i = 0; i < p_list1->count; i++)
	{
		tg_list_insert(p_list0, TG_LIST_POINTER_TO(*p_list1, i));
	}
}

void tg_list_insert_unchecked(tg_list* p_list, const void* p_value)
{
	TG_ASSERT(p_list && p_value && p_list->capacity != p_list->count);

	TG_LIST_SET_AT(*p_list, p_list->count, p_value);
	p_list->count++;
}

void tg_list_remove(tg_list* p_list, const void* p_value)
{
	TG_ASSERT(p_list && p_value);

	for (u32 i = 0; i < p_list->count; i++)
	{
		if (TG_LIST_POINTER_TO(*p_list, i) == p_value)
		{
			tg_list_remove_at(p_list, i);
			return;
		}
	}
}

void tg_list_remove_at(tg_list* p_list, u32 index)
{
	TG_ASSERT(p_list && index <= p_list->count);

	for (u32 i = index + 1; i < p_list->count; i++)
	{
		TG_LIST_SET_AT(*p_list, i - 1, TG_LIST_POINTER_TO(*p_list, i));
	}
	p_list->count--;
}

void tg_list_replace_at(tg_list* p_list, u32 index, const void* p_value)
{
	TG_ASSERT(p_list && index < p_list->count && p_value);

	TG_LIST_SET_AT(*p_list, index, p_value);
}

void tg_list_reserve(tg_list* p_list, u32 capacity)
{
	TG_ASSERT(p_list && capacity > p_list->capacity);

	p_list->capacity = capacity;
	p_list->elements = TG_MEMORY_REALLOC(p_list->elements, (u64)capacity * (u64)p_list->element_size);
}
