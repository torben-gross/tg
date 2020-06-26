#ifndef TG_MEMORY_H
#define TG_MEMORY_H

#include "tg_common.h"

#ifndef TG_DEBUG
#include "platform/tg_platform.h"
#endif

#ifdef TG_DEBUG

#define    TG_MEMORY_ALLOC(size)                        tg_memory_alloc_impl(size, __FILE__, __LINE__, TG_FALSE)
#define    TG_MEMORY_ALLOC_NULLIFY(size)                tg_memory_alloc_impl(size, __FILE__, __LINE__, TG_TRUE)
#define    TG_MEMORY_REALLOC(size, p_memory)            tg_memory_realloc_impl(size, p_memory, __FILE__, __LINE__, TG_FALSE)
#define    TG_MEMORY_REALLOC_NULLIFY(size, p_memory)    tg_memory_realloc_impl(size, p_memory, __FILE__, __LINE__, TG_TRUE)
#define    TG_MEMORY_FREE(p_memory)                     tg_memory_free_impl(p_memory, __FILE__, __LINE__)

#else

#define    TG_MEMORY_ALLOC(size)                        tg_platform_memory_alloc((u64)size)
#define    TG_MEMORY_ALLOC_NULLIFY(size)                tg_platform_memory_alloc_nullify((u64)size)
#define    TG_MEMORY_REALLOC(size, p_memory)            tg_platform_memory_realloc((u64)size, p_memory)
#define    TG_MEMORY_REALLOC_NULLIFY(size, p_memory)    tg_platform_memory_realloc_nullify((u64)size, p_memory)
#define    TG_MEMORY_FREE(p_memory)                     tg_platform_memory_free(p_memory);

#endif

#define    TG_MEMORY_STACK_ALLOC(size)                  tg_memory_stack_alloc(size);
#define    TG_MEMORY_STACK_FREE(size)                   tg_memory_stack_free(size);



#ifdef TG_DEBUG
TG_DECLARE_TYPE(tg_list);

typedef struct tg_memory_allocation
{
	char    p_filename[256];
	u32     line;
} tg_memory_allocator_allocation;
#endif



void       tg_memory_init();
void       tg_memory_shutdown();



void       tg_memory_copy(u64 size, const void* p_source, void* p_destination);
void       tg_memory_nullify(u64 size, void* p_memory);
void       tg_memory_set_all_bits(u64 size, void* p_memory);

#ifdef TG_DEBUG

void*      tg_memory_alloc_impl(u64 size, const char* p_filename, u32 line, b32 nullify);
void*      tg_memory_realloc_impl(u64 size, void* p_memory, const char* p_filename, u32 line, b32 nullify);
void       tg_memory_free_impl(void* p_memory, const char* p_filename, u32 line);
u32        tg_memory_unfreed_allocation_count();
tg_list    tg_memory_create_unfreed_allocations_list();

#endif



void*      tg_memory_stack_alloc(u64 size);
void       tg_memory_stack_free(u64 size);

#endif
