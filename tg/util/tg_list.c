#include "tg/util/tg_list.h"

#include "tg/memory/tg_memory_allocator.h"
#include <string.h> // TODO: memcpy

typedef struct tg_list
{
	u32    capacity;
	u32    element_size;
	u32    count;
	u8*    elements;
} tg_list;

void tg_list_create_impl(u32 capacity, u32 element_size, tg_list_h* p_list_h)
{
	TG_ASSERT(capacity && element_size && p_list_h);

	*p_list_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_list_h));

	(**p_list_h).capacity = capacity;
	(**p_list_h).element_size = element_size;
	(**p_list_h).elements = TG_MEMORY_ALLOCATOR_ALLOCATE((u64)capacity * (u64)element_size);
}

void tg_list_reserve(tg_list_h list_h, u32 capacity)
{
	TG_ASSERT(list_h && capacity > list_h->capacity);

	list_h->capacity = capacity;
	TG_MEMORY_ALLOCATOR_REALLOCATE(list_h->elements, capacity * list_h->element_size);
}

void tg_list_insert(tg_list_h list_h, const void* p_element)
{
	TG_ASSERT(list_h && p_element);

	if (list_h->capacity == list_h->count)
	{
		const u32 new_capacity = 2 * list_h->capacity;
		tg_list_reserve(list_h, new_capacity);
	}

	memcpy(&list_h->elements[list_h->count * list_h->element_size], p_element, list_h->element_size);
	list_h->count++;
}

void tg_list_insert_unchecked(tg_list_h list_h, const void* p_element)
{
	TG_ASSERT(list_h && p_element && list_h->capacity != list_h->count);

	memcpy(&list_h->elements[list_h->count * list_h->element_size], p_element, list_h->element_size);
	list_h->count++;
}

void* tg_list_at(tg_list_h list_h, u32 index)
{
	TG_ASSERT(list_h && index < list_h->count);

	return &list_h->elements[index * list_h->element_size];
}

void tg_list_destroy(tg_list_h list_h)
{
	TG_ASSERT(list_h);

	TG_MEMORY_ALLOCATOR_FREE(list_h->elements);
	TG_MEMORY_ALLOCATOR_FREE(list_h);
}
