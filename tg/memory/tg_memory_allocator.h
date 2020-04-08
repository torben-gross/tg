#ifndef TG_ALLOCATOR_H
#define TG_ALLOCATOR_H

#include "tg/tg_common.h"

#ifndef TG_DEBUG
#include <stdlib.h>
#endif

#ifdef TG_DEBUG
#define    TG_MEMORY_ALLOCATOR_ALLOCATE(size)    tg_allocator_allocate_impl(size)
#define    TG_MEMORY_ALLOCATOR_FREE(p_memory)    tg_allocator_free_impl(p_memory)
#else
#define    TG_MEMORY_ALLOCATOR_ALLOCATE(size)    malloc((size_t)size)
#define    TG_MEMORY_ALLOCATOR_FREE(p_memory)    free(p_memory);
#endif

#ifdef TG_DEBUG
void*    tg_allocator_allocate_impl(u64 size);
void     tg_allocator_free_impl(void* p_memory);
u64      tg_allocator_unfreed_allocation_count();
#endif

#endif
