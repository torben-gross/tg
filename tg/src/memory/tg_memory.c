#include "memory/tg_memory.h"

#ifdef TG_DEBUG
#include "util/tg_hashmap.h"
#include "util/tg_string.h"
#include <memory.h>
#include <stdlib.h>
#endif



#define TG_MEMORY_STACK_SIZE    (1LL << 30LL)



typedef struct tg_memory_stack
{
	u64    exhausted_size;
	u8*    p_memory;
} tg_memory_stack;



#ifdef TG_DEBUG
b32                recording_allocations = TG_FALSE;
tg_hashmap         memory_allocations = { 0 };
#endif

tg_memory_stack    memory_stack = { 0 };



void tg_memory_init()
{
#ifdef TG_DEBUG
	memory_allocations = TG_HASHMAP_CREATE(void*, tg_memory_allocator_allocation);
#endif

	memory_stack.exhausted_size = 0;
	memory_stack.p_memory = malloc(TG_MEMORY_STACK_SIZE);
	TG_ASSERT(memory_stack.p_memory);

#ifdef TG_DEBUG
	recording_allocations = TG_TRUE;
#endif
}

void tg_memory_shutdown()
{
#ifdef TG_DEBUG
	recording_allocations = TG_FALSE;
#endif

	free(memory_stack.p_memory);
	memory_stack.exhausted_size = 0;

#ifdef TG_DEBUG
	tg_hashmap_destroy(&memory_allocations);
#endif
}



#ifdef TG_DEBUG

void* tg_memory_alloc_impl(u64 size, const char* p_filename, u32 line)
{
	void* p_memory = malloc((size_t)size);
	TG_ASSERT(p_memory);
	memset(p_memory, 0, (size_t)size);

	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		memcpy(allocation.p_filename, p_filename, tg_string_length(p_filename) * sizeof(*p_filename));
		tg_hashmap_insert(&memory_allocations, &p_memory, &allocation);
		recording_allocations = TG_TRUE;
	}

	return p_memory;
}

void* tg_memory_realloc_impl(void* p_memory, u64 size, const char* p_filename, u32 line)
{
	void* p_reallocated_memory = realloc(p_memory, size);
	TG_ASSERT(p_reallocated_memory);

	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_hashmap_remove(&memory_allocations, &p_memory);
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		memcpy(allocation.p_filename, p_filename, tg_string_length(p_filename) * sizeof(*p_filename));
		tg_hashmap_insert(&memory_allocations, &p_reallocated_memory, &allocation);
		recording_allocations = TG_TRUE;
	}

	return p_reallocated_memory;
}

void tg_memory_free_impl(void* p_memory, const char* p_filename, u32 line)
{
	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_hashmap_remove(&memory_allocations, &p_memory);
		recording_allocations = TG_TRUE;
	}

	free(p_memory);
}

u32 tg_memory_unfreed_allocation_count()
{
	return tg_hashmap_element_count(&memory_allocations);
}

tg_list tg_memory_create_unfreed_allocations_list()
{
	recording_allocations = TG_FALSE;
	const tg_list list = tg_hashmap_value_list_create(&memory_allocations);
	recording_allocations = TG_TRUE;
	return list;
}

#endif



void* tg_memory_stack_alloc(u64 size)
{
	TG_ASSERT(size && memory_stack.exhausted_size + size < TG_MEMORY_STACK_SIZE);

	void* memory = &memory_stack.p_memory[memory_stack.exhausted_size];
	memory_stack.exhausted_size += size;
	return memory;
}

void tg_memory_stack_free(u64 size)
{
	TG_ASSERT(size && memory_stack.exhausted_size >= size);

	memory_stack.exhausted_size -= size;
}
