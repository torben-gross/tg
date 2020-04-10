#ifndef TG_ALLOCATOR_H
#define TG_ALLOCATOR_H

#include "tg_common.h"

#ifndef TG_DEBUG
#include <stdlib.h>
#endif

#ifdef TG_DEBUG
#define    TG_MEMORY_ALLOCATOR_ALLOCATE(size)                tg_allocator_allocate_impl(size, __FILE__, __LINE__)
#define    TG_MEMORY_ALLOCATOR_REALLOCATE(p_memory, size)    tg_allocator_reallocate_impl(p_memory, size, __FILE__, __LINE__);
#define    TG_MEMORY_ALLOCATOR_FREE(p_memory)                tg_allocator_free_impl(p_memory, __FILE__, __LINE__)
#else
#define    TG_MEMORY_ALLOCATOR_ALLOCATE(size)                malloc((size_t)size)
#define    TG_MEMORY_ALLOCATOR_REALLOCATE(p_memory, size)    realloc(p_memory, (size_t)size);
#define    TG_MEMORY_ALLOCATOR_FREE(p_memory)                free(p_memory);
#endif



#ifdef TG_DEBUG
TG_DECLARE_HANDLE(tg_list);

typedef struct tg_memory_allocation
{
	char    p_filename[256];
	u32     line;
} tg_memory_allocator_allocation;
#endif



void         tg_memory_init();
void         tg_memory_shutdown();

#ifdef TG_DEBUG
void*        tg_allocator_allocate_impl(u64 size, const char* p_filename, u32 line);
void*        tg_allocator_reallocate_impl(void* p_memory, u64 size, const char* p_filename, u32 line);
void         tg_allocator_free_impl(void* p_memory, const char* p_filename, u32 line);
u32          tg_allocator_unfreed_allocation_count();
tg_list_h    tg_allocator_create_unfreed_allocations_list();
#endif

#endif
