#ifndef TG_ALLOCATOR_H
#define TG_ALLOCATOR_H

#include "tg_platform.h"
#include "tg/tg_common.h"

#ifdef TG_DEBUG

void* tg_allocator_allocate(ui64 size);
void tg_allocator_free(void* memory);
ui64 tg_allocator_unfreed_allocation_count();

#else

#include <stdlib.h>

#define tg_allocator_allocate(size) malloc((size_t)size)
#define tg_allocator_free(memory) free(memory);

#endif

#endif
