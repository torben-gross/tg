#include "tg_allocator.h"

#ifdef TG_DEBUG

#include <memory.h>
#include <stdlib.h>

ui64 total_allocation_count = 0;

void* tg_allocator_allocate_impl(ui64 size)
{
	total_allocation_count++;

	void* memory = malloc((size_t)size);
	TG_ASSERT(memory);
	memset(memory, 0, (size_t)size);
	return memory;
}

void tg_allocator_free_impl(void* memory)
{
	total_allocation_count--;

	free(memory);
}

ui64 tg_allocator_unfreed_allocation_count()
{
	return total_allocation_count;
}
#endif
