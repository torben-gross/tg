#include "memory/tg_memory.h"

#ifdef TG_DEBUG
#include "platform/tg_platform.h"
#include "util/tg_hashmap.h"
#include "util/tg_string.h"
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
	memory_stack.p_memory = tg_platform_memory_alloc(TG_MEMORY_STACK_SIZE);
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

	tg_platform_memory_free(memory_stack.p_memory);
	memory_stack.exhausted_size = 0;

#ifdef TG_DEBUG
	tg_hashmap_destroy(&memory_allocations);
#endif
}



void tg_memory_copy(u64 size, const void* p_source, void* p_destination)
{
	for (u64 i = 0; i < size; i++)
	{
		((u8*)p_destination)[i] = ((u8*)p_source)[i];
	}
}

void tg_memory_nullify(u64 size, void* p_memory)
{
	for (u64 i = 0; i < size; i++)
	{
		((u8*)p_memory)[i] = 0;
	}
}

#ifdef TG_DEBUG

void* tg_memory_alloc_impl(u64 size, const char* p_filename, u32 line)
{
	void* p_memory = tg_platform_memory_alloc((size_t)size);
	TG_ASSERT(p_memory);
	tg_memory_nullify(size, p_memory);

	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		tg_memory_copy(tg_string_length(p_filename) * sizeof(*p_filename), p_filename, allocation.p_filename);
		tg_hashmap_insert(&memory_allocations, &p_memory, &allocation);
		recording_allocations = TG_TRUE;
	}

	return p_memory;
}

void* tg_memory_realloc_impl(u64 size, void* p_memory, const char* p_filename, u32 line)
{
	void* p_reallocated_memory = tg_platform_memory_realloc(size, p_memory);
	TG_ASSERT(p_reallocated_memory);

	if (recording_allocations)
	{
		recording_allocations = TG_FALSE;
		tg_hashmap_remove(&memory_allocations, &p_memory);
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		tg_memory_copy(tg_string_length(p_filename) * sizeof(*p_filename), p_filename, allocation.p_filename);
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
		// TODO: this needs to 'try', because the platform layer calls
		// 'tg_memory_create_unfreed_allocations_list', which disables recording
		// temporarily and thus, the hashmap does not contain it.
		tg_hashmap_try_remove(&memory_allocations, &p_memory);
		recording_allocations = TG_TRUE;
	}

	tg_platform_memory_free(p_memory);
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
#ifdef TG_DEBUG
	tg_memory_nullify(size, &memory_stack.p_memory[memory_stack.exhausted_size]);
#endif
}
