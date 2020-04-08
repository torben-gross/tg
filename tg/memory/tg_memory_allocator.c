#include "tg/memory/tg_memory_allocator.h"

#ifdef TG_DEBUG

#include <memory.h>
#include <stdlib.h>

u64 total_allocation_count = 0;

void* tg_allocator_allocate_impl(u64 size)
{
	total_allocation_count++;

	void* memory = malloc((size_t)size);
	TG_ASSERT(memory);
	memset(memory, 0, (size_t)size);
	return memory;
}

void tg_allocator_free_impl(void* p_memory)
{
	total_allocation_count--;

	free(p_memory);
}

u64 tg_allocator_unfreed_allocation_count()
{
	return total_allocation_count;
}
#endif
