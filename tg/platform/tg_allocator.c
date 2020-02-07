#include "tg_allocator.h"

#ifdef TG_DEBUG
void* glob_alloc(size_t size)
{
	void* memory = malloc(size);
	ASSERT(memory);
	return memory;
}
#endif
