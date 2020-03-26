#ifndef TG_ALLOCATOR_H
#define TG_ALLOCATOR_H

#include "tg_platform.h"
#include "tg/tg_common.h"

#ifndef TG_DEBUG
#include <stdlib.h>
#endif

#ifdef TG_DEBUG
#define    TG_ALLOCATOR_ALLOCATE(size)    tg_allocator_allocate_impl(size)
#define    TG_ALLOCATOR_FREE(memory)      tg_allocator_free_impl(memory)
#else
#define    TG_ALLOCATOR_ALLOCATE(size)    malloc((size_t)size)
#define    TG_ALLOCATOR_FREE(memory)      free(memory);
#endif

#ifdef TG_DEBUG
void*    tg_allocator_allocate_impl(ui64 size);
void     tg_allocator_free_impl(void* memory);
ui64     tg_allocator_unfreed_allocation_count();
#endif

#endif
