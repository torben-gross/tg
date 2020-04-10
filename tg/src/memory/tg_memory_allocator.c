#include "memory/tg_memory_allocator.h"

#ifdef TG_DEBUG

#include "util/tg_hashmap.h"
#include "util/tg_string.h"
#include <memory.h>
#include <stdlib.h>



b32             recording_allocations = TG_FALSE;
tg_hashmap_h    memory_allocations = TG_NULL;



u32 tg_memory_allocator_internal_voidpointer_hash(const void** pp_v) // Knuth's Multiplicative Method
{
	TG_ASSERT(pp_v);

	const u32 result = (u32)*(u64*)pp_v * 2654435761;
	return result;
}

b32 tg_memory_allocator_internal_voidpointer_equals(const void** pp_v0, const void** pp_v1)
{
	TG_ASSERT(pp_v0 && pp_v1);

	const void* p_v0 = *pp_v0;
	const void* p_v1 = *pp_v1;
	return p_v0 == p_v1;
}



void tg_memory_init()
{
	memory_allocations = tg_hashmap_create_count_capacity(
		void*, tg_memory_allocator_allocation,
		1024, 4,
		tg_memory_allocator_internal_voidpointer_hash, tg_memory_allocator_internal_voidpointer_equals
	);
	recording_allocations = TG_TRUE;
}

void tg_memory_shutdown()
{
	recording_allocations = TG_FALSE;
	tg_hashmap_destroy(memory_allocations);
}



void* tg_allocator_allocate_impl(u64 size, const char* p_filename, u32 line)
{
	void* memory = malloc((size_t)size);
	TG_ASSERT(memory);
	memset(memory, 0, (size_t)size);

	if (recording_allocations)
	{
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		memcpy(allocation.p_filename, p_filename, tg_string_length(p_filename) * sizeof(*p_filename));
		tg_hashmap_insert(memory_allocations, &memory, &allocation);
	}


	return memory;
}

void* tg_allocator_reallocate_impl(void* p_memory, u64 size, const char* p_filename, u32 line)
{
	void* p_reallocated_memory = realloc(p_memory, size);
	TG_ASSERT(p_reallocated_memory);

	if (recording_allocations)
	{
		tg_hashmap_remove(memory_allocations, &p_memory);
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		memcpy(allocation.p_filename, p_filename, tg_string_length(p_filename) * sizeof(*p_filename));
		tg_hashmap_insert(memory_allocations, &p_reallocated_memory, &allocation);
	}

	return p_reallocated_memory;
}

void tg_allocator_free_impl(void* p_memory, const char* p_filename, u32 line)
{
	if (recording_allocations)
	{
		tg_hashmap_try_remove(memory_allocations, &p_memory);
	}

	free(p_memory);
}

u32 tg_allocator_unfreed_allocation_count()
{
	return tg_hashmap_element_count(memory_allocations);
}

tg_list_h tg_allocator_create_unfreed_allocations_list()
{
	recording_allocations = TG_FALSE;
	const tg_list_h list_h = tg_hashmap_create_value_list(memory_allocations);
	recording_allocations = TG_TRUE;
	return list_h;
}

#endif
