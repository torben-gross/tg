#ifndef TG_MEMORY_H
#define TG_MEMORY_H

#include "platform/tg_platform.h"
#include "tg_common.h"

#ifdef TG_DEBUG

#define TG_MEMORY_ALLOC(size)                                      tg_memory_alloc_impl(size, __FILE__, __LINE__, TG_FALSE)
#define TG_MEMORY_ALLOC_NULLIFY(size)                              tg_memory_alloc_impl(size, __FILE__, __LINE__, TG_TRUE)
#define TG_MEMORY_REALLOC(size, p_memory)                          tg_memory_realloc_impl(size, p_memory, __FILE__, __LINE__, TG_FALSE)
#define TG_MEMORY_REALLOC_NULLIFY(size, p_memory)                  tg_memory_realloc_impl(size, p_memory, __FILE__, __LINE__, TG_TRUE)
#define TG_MEMORY_FREE(p_memory)                                   tg_memory_free_impl(p_memory)

#else

#define TG_MEMORY_ALLOC(size)                                      tgp_malloc((u64)size)
#define TG_MEMORY_ALLOC_NULLIFY(size)                              tgp_malloc_nullify((u64)size)
#define TG_MEMORY_REALLOC(size, p_mapped_device_memory)            tgp_realloc((u64)size, p_mapped_device_memory)
#define TG_MEMORY_REALLOC_NULLIFY(size, p_mapped_device_memory)    tgp_realloc_nullify((u64)size, p_mapped_device_memory)
#define TG_MEMORY_FREE(p_mapped_device_memory)                     tgp_free(p_mapped_device_memory)

#endif

#define TG_MEMORY_STACK_ALLOC(size)                                tg_memory_stack_alloc(size)
#define TG_MEMORY_STACK_FREE(size)                                 tg_memory_stack_free(size)
#define TG_MEMORY_STACK_ALLOC_ASYNC(size)                          tg_memory_stack_alloc_async(size)
#define TG_MEMORY_STACK_FREE_ASYNC(size)                           tg_memory_stack_free_async(size)



void       tg_memory_init(void);
void       tg_memory_shutdown(void);



void       tg_memcpy(u64 size, const void* p_source, void* p_destination);
void       tg_memory_nullify(u64 size, void* p_memory);
void       tg_memory_set_all_bits(u64 size, void* p_memory);

#ifdef TG_DEBUG

void*      tg_memory_alloc_impl(u64 size, const char* p_filename, u32 line, b32 nullify);
void*      tg_memory_realloc_impl(u64 size, void* p_memory, const char* p_filename, u32 line, b32 nullify);
void       tg_memory_free_impl(void* p_memory);
u32        tg_memory_active_allocation_count(void);

#endif

void*      tg_memory_stack_alloc(u64 size);
void       tg_memory_stack_free(u64 size);
void*      tg_memory_stack_alloc_async(u64 size);
void       tg_memory_stack_free_async(u64 size);

#endif
