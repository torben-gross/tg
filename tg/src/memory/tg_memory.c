#include "memory/tg_memory.h"

#ifndef TG_DEBUG

void tg_memory_init()     { }
void tg_memory_shutdown() { }

#else

#include "util/tg_hashmap.h"
#include "util/tg_string.h"
#include <memory.h>
#include <stdlib.h>



b32             recording_allocations = TG_FALSE;
tg_hashmap_h    memory_allocations = TG_NULL;



void tg_memory_init()
{
	memory_allocations = TG_HASHMAP_CREATE__BUCKET_COUNT__BUCKET_CAPACITY(void*, tg_memory_allocator_allocation, 997, 4);
	recording_allocations = TG_TRUE;
}

void tg_memory_shutdown()
{
	recording_allocations = TG_FALSE;
	tg_hashmap_destroy(memory_allocations);
}



void* tg_memory_alloc_impl(u64 size, const char* p_filename, u32 line)
{
	void* p_memory = malloc((size_t)size);
	TG_ASSERT(p_memory);
	memset(p_memory, 0, (size_t)size);

	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		memcpy(allocation.p_filename, p_filename, tg_string_length(p_filename) * sizeof(*p_filename));
		tg_hashmap_insert(memory_allocations, &p_memory, &allocation);
		recording_allocations = TG_TRUE;
	}

	return p_memory;
}

void* tg_memory_realloc_impl(void* p_memory, u64 size, const char* p_filename, u32 line)
{
	void* p_reallocated_memory = realloc(p_memory, size);
	TG_ASSERT(p_reallocated_memory);

	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_hashmap_remove(memory_allocations, &p_memory);
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		memcpy(allocation.p_filename, p_filename, tg_string_length(p_filename) * sizeof(*p_filename));
		tg_hashmap_insert(memory_allocations, &p_reallocated_memory, &allocation);
		recording_allocations = TG_TRUE;
	}

	return p_reallocated_memory;
}

void tg_memory_free_impl(void* p_memory, const char* p_filename, u32 line)
{
	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_hashmap_try_remove(memory_allocations, &p_memory);
		recording_allocations = TG_TRUE;
	}

	free(p_memory);
}

u32 tg_memory_unfreed_allocation_count()
{
	return tg_hashmap_element_count(memory_allocations);
}

tg_list_h tg_memory_create_unfreed_allocations_list()
{
	recording_allocations = TG_FALSE;
	const tg_list_h list_h = tg_hashmap_value_list_create(memory_allocations);
	recording_allocations = TG_TRUE;
	return list_h;
}

#endif
