#ifndef TG_MEMORY_H
#define TG_MEMORY_H

#include "platform/tg_platform.h"
#include "tg_common.h"

#ifdef TG_DEBUGd
#define TG_MALLOC(size)                             tg_memory_alloc_impl(size, __FILE__, __LINE__)
#define TG_REALLOC(size, p_memory)                  tg_memory_realloc_impl(size, p_memory, __FILE__, __LINE__)
#define TG_FREE(p_memory)                           tg_memory_free_impl(p_memory)
#else
#define TG_MALLOC(size)                             tgp_malloc(size)
#define TG_REALLOC(size, p_mapped_device_memory)    tgp_realloc(size, p_mapped_device_memory)
#define TG_FREE(p_mapped_device_memory)             tgp_free(p_mapped_device_memory)
#endif

#define TG_MALLOC_STACK(size)                       tg_memory_stack_alloc(size)
#define TG_FREE_STACK(size)                         tg_memory_stack_free(size)
#define TG_MALLOC_STACK_ASYNC(size)                 tg_memory_stack_alloc_async(size)
#define TG_FREE_STACK_ASYNC(size)                   tg_memory_stack_free_async(size)



void     tg_memory_init(void);
void     tg_memory_shutdown(void);



void     tg_memcpy(tg_size size, const void* p_source, void* p_destination);
void     tg_memory_nullify(tg_size size, void* p_memory);
void     tg_memory_set_all_bits(tg_size size, void* p_memory);

#ifdef TG_DEBUG

void*    tg_memory_alloc_impl(tg_size size, const char* p_filename, u32 line);
void*    tg_memory_realloc_impl(tg_size size, void* p_memory, const char* p_filename, u32 line);
void     tg_memory_free_impl(void* p_memory);
u32      tg_memory_active_allocation_count(void);

#endif

void*    tg_memory_stack_alloc(tg_size size);
void     tg_memory_stack_free(tg_size size);
void*    tg_memory_stack_alloc_async(tg_size size);
void     tg_memory_stack_free_async(tg_size size);

#endif
