#include "util/tg_list.h"

#include "memory/tg_memory.h"
#include <string.h> // TODO: memcpy


#define TG_LIST_GET_POINTER_AT(list_h, index)     ((void*)&list_h->elements[(index) * list_h->element_size])
#define TG_LIST_SET_AT(list_h, index, p_value)    memcpy(TG_LIST_GET_POINTER_AT(list_h, index), p_value, list_h->element_size)


typedef struct tg_list
{
	u32    element_size;
	u32    capacity;
	u32    count;
	u8*    elements;
} tg_list;



tg_list_h tg_list_create_impl(u32 element_size, u32 capacity)
{
	TG_ASSERT(element_size && capacity);

	tg_list_h list_h = TG_MEMORY_ALLOC(sizeof(*list_h));

	list_h->element_size = element_size;
	list_h->capacity = capacity;
	list_h->count = 0;
	list_h->elements = TG_MEMORY_ALLOC((u64)capacity * (u64)element_size);

	return list_h;
}

void tg_list_destroy(tg_list_h list_h)
{
	TG_ASSERT(list_h);

	TG_MEMORY_FREE(list_h->elements);
	TG_MEMORY_FREE(list_h);
}



u32 tg_list_capacity(const tg_list_h list_h)
{
	TG_ASSERT(list_h);

	return list_h->capacity;
}

b32 tg_list_contains(const tg_list_h list_h, const void* p_value)
{
	TG_ASSERT(list_h && p_value);

	b32 contains = TG_FALSE;
	for (u32 i = 0; i < list_h->count; i++)
	{
		b32 equal = TG_TRUE;
		for (u32 b = 0; b < list_h->element_size; b++)
		{
			equal |= list_h->elements[i * list_h->element_size + b] == ((u8*)p_value)[b];
		}
		contains &= equal;
	}
	return contains;
}

u32 tg_list_count(const tg_list_h list_h)
{
	TG_ASSERT(list_h);

	return list_h->count;
}

void tg_list_insert(tg_list_h list_h, const void* p_value)
{
	TG_ASSERT(list_h && p_value);

	if (list_h->capacity == list_h->count)
	{
		const u32 new_capacity = 2 * list_h->capacity;
		tg_list_reserve(list_h, new_capacity);
	}

	tg_list_insert_unchecked(list_h, p_value);
}

void tg_list_insert_at(tg_list_h list_h, u32 index, const void* p_value)
{
	TG_ASSERT(list_h && index <= list_h->count && p_value);

	if (list_h->capacity == list_h->count)
	{
		const u32 new_capacity = 2 * list_h->capacity;
		tg_list_reserve(list_h, new_capacity);
	}

	tg_list_insert_at_unchecked(list_h, index, p_value);
}

void tg_list_insert_at_unchecked(tg_list_h list_h, u32 index, const void* p_value)
{
	TG_ASSERT(list_h && index <= list_h->count && list_h->capacity > list_h->count && p_value);

	for (u32 i = list_h->count - 1; i >= index; i--)
	{
		TG_LIST_SET_AT(list_h, i + 1, TG_LIST_GET_POINTER_AT(list_h, i));
	}
	TG_LIST_SET_AT(list_h, index, p_value);
	list_h->count++;
}

void tg_list_insert_list(tg_list_h list0_h, const tg_list_h list1_h)
{
	TG_ASSERT(list0_h && list1_h);

	for (u32 i = 0; i < list1_h->count; i++)
	{
		tg_list_insert(list0_h, TG_LIST_GET_POINTER_AT(list1_h, i));
	}
}

void tg_list_insert_unchecked(tg_list_h list_h, const void* p_value)
{
	TG_ASSERT(list_h && p_value && list_h->capacity != list_h->count);

	TG_LIST_SET_AT(list_h, list_h->count, p_value);
	list_h->count++;
}

void* tg_list_pointer_to(tg_list_h list_h, u32 index)
{
	TG_ASSERT(list_h && index < list_h->count);

	return TG_LIST_GET_POINTER_AT(list_h, index);
}

void tg_list_remove(tg_list_h list_h, const void* p_value)
{
	TG_ASSERT(list_h && p_value);

	for (u32 i = 0; i < list_h->count; i++)
	{
		if (TG_LIST_GET_POINTER_AT(list_h, i) == p_value)
		{
			tg_list_remove_at(list_h, i);
			return;
		}
	}
}

void tg_list_remove_at(tg_list_h list_h, u32 index)
{
	TG_ASSERT(list_h && index <= list_h->count);

	for (u32 i = index + 1; i < list_h->count; i++)
	{
		TG_LIST_SET_AT(list_h, i - 1, TG_LIST_GET_POINTER_AT(list_h, i));
	}
	list_h->count--;
}

void tg_list_replace_at(tg_list_h list_h, u32 index, const void* p_value)
{
	TG_ASSERT(list_h && index < list_h->count && p_value);

	TG_LIST_SET_AT(list_h, index, p_value);
}

void tg_list_reserve(tg_list_h list_h, u32 capacity)
{
	TG_ASSERT(list_h && capacity > list_h->capacity);

	list_h->capacity = capacity;
	list_h->elements = TG_MEMORY_REALLOC(list_h->elements, (u64)capacity * (u64)list_h->element_size);
}
