#include "tg_allocator.h"

#ifdef TG_DEBUG

#include <memory.h>
#include <stdlib.h>

void* tg_malloc(ui64 size)
{
	void* memory = malloc((size_t)size);
	TG_ASSERT(memory);
	memset(memory, 0, (size_t)size);
	return memory;
}

void tg_free(void* memory)
{
	free(memory);
}
#endif
