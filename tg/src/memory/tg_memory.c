#include "memory/tg_memory.h"

#ifdef TG_DEBUG
#include "platform/tg_platform.h"
#include "util/tg_hashmap.h"
#include "util/tg_string.h"
#endif



#define TG_MEMORY_STACK_SIZE            (1LL << 30LL)
#define TG_MEMORY_STACK_MAGIC_NUMBER    163ui8



typedef struct tg_memory_stack
{
	u64    exhausted_size;
	u8*    p_memory;
} tg_memory_stack;



#ifdef TG_DEBUG
static volatile b32       recording_allocations = TG_FALSE;
static tg_hashmap         memory_allocations = { 0 };
static tg_lock            lock;
#endif

static tg_memory_stack    memory_stack = { 0 };



void tg_memory_init()
{
#ifdef TG_DEBUG
	lock = tg_platform_lock_create(TG_LOCK_STATE_LOCKED);
	memory_allocations = TG_HASHMAP_CREATE(void*, tg_memory_allocator_allocation);

	memory_stack.exhausted_size = 1;
	memory_stack.p_memory = tg_platform_memory_alloc(TG_MEMORY_STACK_SIZE);
	TG_ASSERT(memory_stack.p_memory);
	
	memory_stack.p_memory[0] = TG_MEMORY_STACK_MAGIC_NUMBER;

	recording_allocations = TG_TRUE;
	tg_platform_unlock(&lock);
#else
	memory_stack.exhausted_size = 0;
	memory_stack.p_memory = tg_platform_memory_alloc(TG_MEMORY_STACK_SIZE);
#endif
}

void tg_memory_shutdown()
{
#ifdef TG_DEBUG
	while (!tg_platform_try_lock(&lock));
	recording_allocations = TG_FALSE;
	TG_ASSERT(memory_stack.exhausted_size == 1);
#endif

	tg_platform_memory_free(memory_stack.p_memory);
	memory_stack.exhausted_size = 0;

#ifdef TG_DEBUG
	tg_hashmap_destroy(&memory_allocations);
	tg_platform_unlock(&lock);
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

void tg_memory_set_all_bits(u64 size, void* p_memory)
{
	for (u64 i = 0; i < size; i++)
	{
		((u8*)p_memory)[i] = 0xff;
	}
}

#ifdef TG_DEBUG

void* tg_memory_alloc_impl(u64 size, const char* p_filename, u32 line, b32 nullify)
{
	TG_ASSERT(size);

	void* p_memory = TG_NULL;

	if (nullify)
	{
		p_memory = tg_platform_memory_alloc_nullify(size);
	}
	else
	{
		p_memory = tg_platform_memory_alloc(size);
	}
	TG_ASSERT(p_memory);

	if (TG_INTERLOCKED_COMPARE_EXCHANGE(&recording_allocations, TG_FALSE, TG_TRUE) == TG_TRUE)
	{
		while (!tg_platform_try_lock(&lock));
		tg_memory_allocator_allocation allocation = { 0 };
		allocation.line = line;
		tg_memory_copy(tg_string_length(p_filename) * sizeof(*p_filename), p_filename, allocation.p_filename);
		tg_hashmap_insert(&memory_allocations, &p_memory, &allocation);
		recording_allocations = TG_TRUE;
		tg_platform_unlock(&lock);
	}

	return p_memory;
}

void* tg_memory_realloc_impl(u64 size, void* p_memory, const char* p_filename, u32 line, b32 nullify)
{
	TG_ASSERT(size);
	
	void* p_reallocated_memory = TG_NULL;
	if (p_memory)
	{
		if (nullify)
		{
			p_reallocated_memory = tg_platform_memory_realloc_nullify(size, p_memory);
		}
		else
		{
			p_reallocated_memory = tg_platform_memory_realloc(size, p_memory);
		}
		TG_ASSERT(p_reallocated_memory);

		if (TG_INTERLOCKED_COMPARE_EXCHANGE(&recording_allocations, TG_FALSE, TG_TRUE) == TG_TRUE)
		{
			while (!tg_platform_try_lock(&lock));
			tg_hashmap_try_remove(&memory_allocations, &p_memory); // TODO: this should not 'try', but this implementation is not properly thread save!
			tg_memory_allocator_allocation allocation = { 0 };
			allocation.line = line;
			tg_memory_copy(tg_string_length(p_filename) * sizeof(*p_filename), p_filename, allocation.p_filename);
			tg_hashmap_insert(&memory_allocations, &p_reallocated_memory, &allocation);
			recording_allocations = TG_TRUE;
			tg_platform_unlock(&lock);
		}
		tg_platform_unlock(&lock);
	}
	else
	{
		p_reallocated_memory = tg_memory_alloc_impl(size, p_filename, line, nullify);
	}

	return p_reallocated_memory;
}

void tg_memory_free_impl(void* p_memory, const char* p_filename, u32 line)
{
	if (TG_INTERLOCKED_COMPARE_EXCHANGE(&recording_allocations, TG_FALSE, TG_TRUE) == TG_TRUE)
	{
		while (!tg_platform_try_lock(&lock));
		// TODO: this needs to 'try', because the platform layer calls
		// 'tg_memory_create_unfreed_allocations_list', which disables recording
		// temporarily and thus, the hashmap does not contain it.
		tg_hashmap_try_remove(&memory_allocations, &p_memory);
		recording_allocations = TG_TRUE;
		tg_platform_unlock(&lock);
	}
	tg_platform_unlock(&lock);

	tg_platform_memory_free(p_memory);
}

u32 tg_memory_unfreed_allocation_count()
{
	return tg_hashmap_element_count(&memory_allocations);
}

tg_list tg_memory_create_unfreed_allocations_list()
{
	while (!tg_platform_try_lock(&lock));
	recording_allocations = TG_FALSE;
	const tg_list list = tg_hashmap_value_list_create(&memory_allocations);
	recording_allocations = TG_TRUE;
	tg_platform_unlock(&lock);
	return list;
}

#endif



void* tg_memory_stack_alloc(u64 size)
{
#ifdef TG_DEBUG
	TG_ASSERT(tg_platform_get_current_thread_id() == 0);
	TG_ASSERT(size && memory_stack.exhausted_size + size + 1 < TG_MEMORY_STACK_SIZE);

	void* memory = &memory_stack.p_memory[memory_stack.exhausted_size];
	memory_stack.exhausted_size += size + 1;
	memory_stack.p_memory[memory_stack.exhausted_size - 1] = TG_MEMORY_STACK_MAGIC_NUMBER;

	return memory;
#else
	void* memory = &memory_stack.p_memory[memory_stack.exhausted_size];
	memory_stack.exhausted_size += size;

	return memory;
#endif
}

void tg_memory_stack_free(u64 size)
{
#ifdef TG_DEBUG
	TG_ASSERT(tg_platform_get_current_thread_id() == 0);
	TG_ASSERT(size && memory_stack.exhausted_size - 1 >= size + 1);

	TG_ASSERT(memory_stack.p_memory[memory_stack.exhausted_size - 1] == TG_MEMORY_STACK_MAGIC_NUMBER);
	memory_stack.exhausted_size -= size + 1;
	TG_ASSERT(memory_stack.p_memory[memory_stack.exhausted_size - 1] == TG_MEMORY_STACK_MAGIC_NUMBER);
#else
	memory_stack.exhausted_size -= size;
#endif
}
